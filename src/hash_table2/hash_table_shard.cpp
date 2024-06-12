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

#include <iostream>
#include <sstream>
#include <numeric>
#include "config.h"
#include "hash_table_shard.h"
#include "logger/logger.h"

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

using namespace std;

namespace hash_table2 {

	hash_table_shard::hash_table_shard(const string &db_name, size_t shard_id, size_t hash_table_size,
			const std::string &data_path)
	: hash_table_shard_base(db_name, shard_id, hash_table_size, data_path)
	{
	}

	hash_table_shard::~hash_table_shard() {

	}

	bool hash_table_shard::has(uint64_t key) const {

		std::ifstream reader(filename_pos(), std::ios::binary);

		const size_t hash_pos = key % this->m_hash_table_size;
		reader.seekg(hash_pos * sizeof(size_t));

		// Read page pos.
		size_t page_pos = SIZE_MAX;
		reader.read((char *)&page_pos, sizeof(size_t));

		if (page_pos == SIZE_MAX) return false;

		// Read page.
		size_t page_len;
		reader.seekg(this->hash_table_byte_size() + page_pos, std::ios::beg);
		reader.read((char *)&page_len, sizeof(size_t));

		std::vector<std::array<uint64_t, 3>> page(page_len);
		reader.read((char *)page.data(), page_len * sizeof(std::array<uint64_t, 3>));

		// Find key among pages.
		for (const auto &page_item : page) {
			if (page_item[0] == key) {
				return true;
			}
		}

		return false;
	}

	string hash_table_shard::find(uint64_t key) const {
		size_t ver;
		return find(key, ver);
	}

	string hash_table_shard::find(uint64_t key, size_t &ver) const {

		std::ifstream reader(filename_pos(), std::ios::binary);

		const size_t hash_pos = key % this->m_hash_table_size;
		reader.seekg(hash_pos * sizeof(size_t));

		// Read page pos.
		size_t page_pos = SIZE_MAX;
		reader.read((char *)&page_pos, sizeof(size_t));

		if (page_pos == SIZE_MAX) return "";

		// Read page.
		size_t page_len;
		reader.seekg(this->hash_table_byte_size() + page_pos, std::ios::beg);
		reader.read((char *)&page_len, sizeof(size_t));

		std::vector<std::array<uint64_t, 3>> page(page_len);
		reader.read((char *)page.data(), page_len * sizeof(std::array<uint64_t, 3>));

		// Find key among pages.
		size_t pos = SIZE_MAX;
		for (const auto &page_item : page) {
			if (page_item[0] == key) {
				pos = page_item[1];
				ver = page_item[2];
			}
		}

		if (pos == SIZE_MAX) return "";

		return data_at_position(pos);
	}

	void hash_table_shard::for_each(std::function<void(uint64_t, std::string)> callback) const {
		ifstream infile(filename_data(), ios::binary);
		infile.seekg(0, ios::beg);

		while (!infile.eof()) {
			size_t key;
			if (!infile.read((char *)&key, sizeof(size_t))) break;
			
			size_t data_len;
			if (!infile.read((char *)&data_len, sizeof(size_t))) break;

			if (key == 0ull) {
				// Skip.
				infile.seekg(data_len, std::ios::cur);
				continue;
			}

			std::unique_ptr<char[]> buffer_allocator;
			try {
				buffer_allocator = std::make_unique<char[]>(data_len);
			} catch (std::bad_alloc &exception) {
				std::cout << "bad_alloc detected: " << exception.what() << " file: " << __FILE__ << " line: " << __LINE__ << std::endl;
				std::cout << "tried to allocate: " << data_len << " bytes" << std::endl;
				break;
			}
			char *buffer = buffer_allocator.get();

			infile.read(buffer, data_len);
			stringstream ss(string(buffer, data_len));

			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(ss);

			stringstream decompressed;
			decompressed << decompress_stream.rdbuf();

			const std::string value = decompressed.str();

			callback(key, std::move(value));
		}
	}

	void hash_table_shard::for_each_key(std::function<void(uint64_t)> callback) const {
		ifstream infile(filename_data(), ios::binary);
		infile.seekg(0, ios::beg);

		while (!infile.eof()) {
			size_t key;
			if (!infile.read((char *)&key, sizeof(size_t))) break;
			
			size_t data_len;
			if (!infile.read((char *)&data_len, sizeof(size_t))) break;

			infile.seekg(data_len, std::ios::cur);

			callback(key);
		}
	}

	size_t hash_table_shard::shard_id() const {
		return m_shard_id;
	}

	size_t hash_table_shard::size() const {
		auto pages = this->read_pages();
		return std::transform_reduce(pages.cbegin(), pages.cend(), 0, [](auto a, auto b) { return a + b; }, [](const auto &p) { return p.size(); });
	}

	size_t hash_table_shard::file_size() const {
		std::ifstream infile(filename_data(), ios::binary);
		infile.seekg(0, std::ios::end);
		return infile.tellg();
	}

	string hash_table_shard::data_at_position(size_t pos) const {

		ifstream infile(filename_data(), ios::binary);
		infile.seekg(pos, ios::beg);

		// Read key
		uint64_t read_key;
		infile.read((char *)&read_key, sizeof(uint64_t));

		// Read data length.
		size_t data_len;
		infile.read((char *)&data_len, sizeof(size_t));

		std::unique_ptr<char[]> buffer_allocator;
		try {
			buffer_allocator = std::make_unique<char[]>(data_len);
		} catch (std::bad_alloc &exception) {
			std::cout << "bad_alloc detected: " << exception.what() << " file: " << __FILE__ << " line: " << __LINE__ << std::endl;
			std::cout << "tried to allocate: " << data_len << " bytes" << std::endl;
			return "";
		}
		char *buffer = buffer_allocator.get();

		infile.read(buffer, data_len);
		stringstream ss(string(buffer, data_len));

		boost::iostreams::filtering_istream decompress_stream;
		decompress_stream.push(boost::iostreams::gzip_decompressor());
		decompress_stream.push(ss);

		stringstream decompressed;
		decompressed << decompress_stream.rdbuf();

		return decompressed.str();
	}

}
