/*
 * MIT License
 *
 * Alexandria.org
 *
 * Copyright (c) 2021 Josef Cullhed, <info@alexandria.org>, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "archive.h"
#include "file.h"
#include "algorithm/algorithm.h"
#include "utils/thread_pool.hpp"
#include <cmath>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <sstream>

namespace file {

	archive::archive(const std::string &filename)
	: m_filename(filename) {

	}

	archive::~archive() {
	
	}

	void archive::read_dir(const std::string &dirname) {

		// Truncate target file.
		std::ofstream outfile(m_filename, std::ios::binary | std::ios::trunc);
		outfile.close();

		boost::filesystem::path path(dirname);

		std::vector<boost::filesystem::path> paths;

		if (is_directory(path)) {
			boost::filesystem::directory_iterator iter(path);
			for (auto &file : boost::make_iterator_range(iter, {})) {
				paths.push_back(file.path());
			}
		}

		std::vector<std::vector<boost::filesystem::path>> chunks;
		algorithm::vector_chunk(paths, std::ceil(paths.size() / m_num_threads) + 1, chunks);

		utils::thread_pool pool(m_num_threads);

		size_t worker_id = 0;
		for (const auto &chunk : chunks) {

			// Remove worker file.
			::file::delete_file(m_filename + "." + std::to_string(worker_id));

			pool.enqueue([this, chunk, worker_id]() {
				for (const auto &path : chunk) {
					add_file(path.generic_string(), path.filename().generic_string(), worker_id);
				}
			});
			worker_id++;
		}

		pool.run_all();

		// Merge workers.
		for (size_t worker_id = 0; worker_id < m_num_threads; worker_id++) {

			std::filebuf infile, outfile;
			
			outfile.open(m_filename, std::ios::out | std::ios::binary | std::ios::app);
			infile.open(m_filename + "." + std::to_string(worker_id), std::ios::in | std::ios::binary);

			std::copy(std::istreambuf_iterator<char>(&infile), {}, std::ostreambuf_iterator<char>(&outfile));

			// Remove worker file.
			::file::delete_file(m_filename + "." + std::to_string(worker_id));
		}
	}

	void archive::untar(const std::string &dest_dir) {
		std::ifstream infile(m_filename, std::ios::binary);

		tar_header header;

		while (!infile.eof()) {
			infile.read((char *)&header, sizeof(tar_header));

			if (infile.eof()) break;

			// This is an unnessecary copy.
			char *buffer = new char[header.m_len];
			infile.read(buffer, header.m_len);

			std::string buffer_string(buffer, header.m_len);
			std::stringstream buffer_stream(buffer_string);

			delete buffer;

			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(buffer_stream);

			std::string decompressed_data(std::istreambuf_iterator<char>(decompress_stream), {});

			std::ofstream outfile(dest_dir + "/" + header.m_filename, std::ios::binary);
			outfile.write(decompressed_data.c_str(), decompressed_data.size());
		}
		
	}

	void archive::untar(std::function<void(const std::string &, const std::string &)> cb) {
		std::ifstream infile(m_filename, std::ios::binary);

		tar_header header;

		while (!infile.eof()) {
			infile.read((char *)&header, sizeof(tar_header));

			if (infile.eof()) break;

			// This is an unnessecary copy.
			char *buffer = new char[header.m_len];
			infile.read(buffer, header.m_len);

			std::string buffer_string(buffer, header.m_len);
			std::stringstream buffer_stream(buffer_string);

			delete buffer;

			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(buffer_stream);

			std::string decompressed_data(std::istreambuf_iterator<char>(decompress_stream), {});

			cb(header.m_filename, decompressed_data);
		}
		
	}

	void archive::add_file(const std::string &path, const std::string &filename, size_t worker_id) {

		std::ofstream outfile(m_filename + "." + std::to_string(worker_id), std::ios::binary | std::ios::app);

		std::string data = ::file::cat(path);

		std::stringstream ss(data);
		boost::iostreams::filtering_istream compress_stream;
		compress_stream.push(boost::iostreams::gzip_compressor());
		compress_stream.push(ss);

		std::string compressed_data(std::istreambuf_iterator<char>(compress_stream), {});

		tar_header header;
		header.m_len = compressed_data.size();
		filename.copy(header.m_filename, filename.size(), 0);
		header.m_filename[filename.size()] = 0;

		outfile.write((char *)&header, sizeof(tar_header));
		outfile.write((char *)compressed_data.c_str(), compressed_data.size());
	}


}
