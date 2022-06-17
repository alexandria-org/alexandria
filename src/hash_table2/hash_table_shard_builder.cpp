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

#include <sstream>
#include "config.h"
#include "hash_table_shard_builder.h"
#include "logger/logger.h"
#include "file/file.h"
#include "indexer/merger.h"

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

using namespace std;

namespace hash_table2 {

	hash_table_shard_builder::hash_table_shard_builder(const string &db_name, size_t shard_id, size_t hash_table_size)
	: hash_table_shard_base(db_name, shard_id, hash_table_size)
	{
		indexer::merger::register_appender((size_t)this, [this]() {append();}, [this]() { return cache_size(); });
		indexer::merger::register_merger((size_t)this, [this]() {merge();});
	}

	hash_table_shard_builder::~hash_table_shard_builder() {
		indexer::merger::deregister_merger((size_t)this);
	}

	void hash_table_shard_builder::add(uint64_t key, const string &value, size_t version) {
		indexer::merger::lock();

		std::lock_guard guard(m_lock);

		auto ver_iter = m_version.find(key);
		if (version > 0 && ver_iter != m_version.end() && ver_iter->second > version) {
			// do nothing
		} else {
			m_data_size += value.capacity();
			m_cache[key] = value;
			m_version[key] = version;
		}
	}

	size_t hash_table_shard_builder::cache_size() const {
		// This is an OK approximation since m_data_size will be much larger than the keys.
		return m_cache.size() * sizeof(uint64_t) * 2 + m_data_size;
	}

	void hash_table_shard_builder::append() {

		std::lock_guard guard(m_lock);

		ofstream outfile(this->filename_data_tmp(), ios::binary | ios::app);

		for (const auto &iter : m_cache) {
			const size_t version = m_version[iter.first];
			outfile.write((char *)&iter.first, sizeof(uint64_t));
			outfile.write((char *)&version, sizeof(size_t));

			// Compress data
			stringstream ss(iter.second);

			boost::iostreams::filtering_istream compress_stream;
			compress_stream.push(boost::iostreams::gzip_compressor());
			compress_stream.push(ss);

			stringstream compressed;
			compressed << compress_stream.rdbuf();

			string compressed_string(compressed.str());

			const size_t data_len = compressed_string.size();
			outfile.write((char *)&data_len, sizeof(size_t));

			outfile.write(compressed_string.c_str(), data_len);
		}

		// Free RAM caches and set m_data_size to zero.
		m_cache = std::map<uint64_t, std::string>{};
		m_version = std::map<uint64_t, size_t>{};
		m_data_size = 0;
	}

	void hash_table_shard_builder::merge() {

		auto pages = this->read_pages();

		const size_t buffer_len = 1024*1024*20;
		
		std::unique_ptr<char[]> buffer_allocator;
		try {
			buffer_allocator = std::make_unique<char[]>(buffer_len);
		} catch (std::bad_alloc &exception) {
			std::cout << "bad_alloc detected: " << exception.what() << " file: " << __FILE__ << " line: " << __LINE__ << std::endl;
			std::cout << "tried to allocate: " << buffer_len << " bytes" << std::endl;
			return;
		}
		char *buffer = buffer_allocator.get();

		// Read append cache and add to pages + data file.
		std::ifstream infile(this->filename_data_tmp(), std::ios::binary);
		std::ofstream outfile(this->filename_data(), std::ios::binary | std::ios::app);

		size_t last_pos = outfile.tellp();

		while (!infile.eof()) {
			uint64_t key;
			if (!infile.read((char *)&key, sizeof(uint64_t))) break;

			size_t version;
			if (!infile.read((char *)&version, sizeof(size_t))) break;

			size_t data_len;
			if (!infile.read((char *)&data_len, sizeof(size_t))) break;

			if (data_len > buffer_len) {
				LOG_INFO("data_len " + to_string(data_len) + "is larger than buffer_len " + to_string(buffer_len) + " in file " + filename_data());
				infile.seekg(data_len, ios::cur);
				continue;
			} else {
				if (!infile.read(buffer, data_len)) break;
			}

			const size_t page_id = key % this->m_hash_table_size;
			const std::array elem{key, last_pos, version};

			auto insert_at = std::upper_bound(pages[page_id].begin(), pages[page_id].end(), elem, [](const auto &a, const auto &b) {
				return a[0] < b[0];
			});

			// insert_at points to the element after "elem"

			bool add_data = false;
			if (pages[page_id].size() == 0) {
				pages[page_id].push_back(elem);
				add_data = true;
			} else {

				const auto elem_at = *(insert_at - 1);
				if (elem_at[0] == elem[0]) {
					// Version is bigger. Replace element.
					if (elem_at[2] <= elem[2]) {
						*(insert_at - 1) = elem;
						add_data = true;
					}
				} else {
					pages[page_id].insert(insert_at, elem);
					add_data = true;
				}
			}

			if (add_data) {
				outfile.write((char *)&key, sizeof(uint64_t));
				outfile.write((char *)&data_len, sizeof(size_t));
				outfile.write(buffer, data_len);

				last_pos += data_len + sizeof(uint64_t) + sizeof(size_t);
			}
		}

		// Truncate cache file.
		ofstream truncate_data_tmp(this->filename_data_tmp(), ios::binary | ios::trunc);
		truncate_data_tmp.close();

		std::ofstream key_writer(this->filename_pos(), std::ios::binary | std::ios::trunc);

		const size_t page_item_size = sizeof(std::array<uint64_t, 3>);
		const size_t empty_key = SIZE_MAX;

		last_pos = 0;
		for (size_t page_id = 0; page_id < pages.size(); page_id++) {
			const size_t page_len = pages[page_id].size();
			if (page_len) {
				key_writer.write((char *)&last_pos, sizeof(size_t));
				last_pos += pages[page_id].size() * page_item_size + sizeof(size_t);
			} else {
				key_writer.write((char *)&empty_key, sizeof(size_t));
			}
		}

		// Write pages.
		for (size_t page_id = 0; page_id < pages.size(); page_id++) {
			const size_t page_len = pages[page_id].size();
			if (page_len) {
				key_writer.write((char *)&page_len, sizeof(size_t));
				for (const auto &page_item : pages[page_id]) {
					key_writer.write((char *)&page_item, page_item_size);
				}
			}
		}
	}

	void hash_table_shard_builder::optimize() {
		auto pages = this->read_pages();

		std::ifstream infile(this->filename_data(), std::ios::binary);
		std::ofstream outfile(this->filename_data_tmp(), std::ios::binary | std::ios::trunc);

		read_optimized_to(pages, infile, outfile);

		outfile.close();

		file::delete_file(filename_data());
		file::delete_file(filename_pos());

		merge();
	}

	void hash_table_shard_builder::truncate() {
		std::lock_guard guard(m_lock);
		ofstream outfile(this->filename_data(), ios::binary | ios::trunc);
		ofstream outfile_pos(this->filename_pos(), ios::binary | ios::trunc);
		ofstream outfile_tmp(this->filename_data_tmp(), ios::binary | ios::trunc);
	}

	void hash_table_shard_builder::merge_with(const hash_table_shard_builder &other) {

		auto pages1 = this->read_pages();
		auto pages2 = other.read_pages();

		// Remove the pages in pages1 that have higher version number in pages2 and vise versa.
		for (size_t p = 0; p < pages1.size(); p++) {
			size_t i = 0, j = 0;
			while (i < pages1[p].size() && j < pages2[p].size()) {
				if (pages1[p][i][0] == pages2[p][j][0]) {
					if (pages1[p][i][2] < pages2[p][j][2]) {
						// delete pages1[p][i]
						pages1[p][i][1] = SIZE_MAX;
					} else {
						// delete pages2[p][j]
						pages2[p][j][1] = SIZE_MAX;
					}
					i++;
					j++;
				} else if (pages1[p][i][0] < pages2[p][j][0]) {
					i++;
				} else {
					j++;
				}
			}
		}

		std::ofstream outfile(this->filename_data_tmp(), std::ios::binary | std::ios::trunc);

		std::ifstream data_file_1(this->filename_data(), std::ios::binary);
		std::ifstream data_file_2(other.filename_data(), std::ios::binary);
		
		read_optimized_to(pages1, data_file_1, outfile);
		read_optimized_to(pages2, data_file_2, outfile);

		outfile.close();

		file::delete_file(filename_data());
		file::delete_file(filename_pos());

		merge();
	}

	void hash_table_shard_builder::read_optimized_to(const std::vector<std::vector<std::array<uint64_t, 3>>> &pages, std::ifstream &infile,
		std::ofstream &outfile) const {
		
		infile.seekg(0, std::ios::beg);

		while (!infile.eof()) {
			const size_t my_pos = infile.tellg();

			size_t key;
			if (!infile.read((char *)&key, sizeof(size_t))) break;
			
			size_t data_len;
			if (!infile.read((char *)&data_len, sizeof(size_t))) break;

			const size_t page_id = key % this->m_hash_table_size;

			std::array elem{key, (uint64_t)0, (uint64_t)0};

			auto iter = std::upper_bound(pages[page_id].cbegin(), pages[page_id].cend(), elem, [](const auto &a, const auto &b) {
				return a[0] < b[0];
			});

			if (pages[page_id].size() == 0) {
				// Skip. Did not find key.
				infile.seekg(data_len, std::ios::cur);
				continue;
			}

			elem = *(iter - 1);

			if (elem[0] == key && elem[1] == my_pos) {

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

				// Keep this data.
				const size_t version = elem[2];
				outfile.write((char *)&key, sizeof(uint64_t));
				outfile.write((char *)&version, sizeof(size_t));
				outfile.write((char *)&data_len, sizeof(size_t));
				outfile.write(buffer, data_len);
			} else {
				infile.seekg(data_len, std::ios::cur);
			}
		}
	}

}
