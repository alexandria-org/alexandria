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

#pragma once

#include <iostream>
#include <memory>
#include <array>
#include <vector>

namespace hash_table2 {

	class hash_table_shard_base {

		public:

			hash_table_shard_base(const std::string &db_name, size_t shard_id, size_t hash_table_size = 1000000,
					const std::string &data_path = "/mnt/{shard_id_mod_8}/hash_table")
			: m_db_name(db_name), m_shard_id(shard_id), m_hash_table_size(hash_table_size), m_data_path(data_path) {}

			std::string file_base_data() const {
				const size_t disk_shard = m_shard_id % 8;
				std::string data_path = m_data_path;
				if (data_path.find("{shard_id_mod_8}") != std::string::npos) {
					data_path.replace(data_path.find("{shard_id_mod_8}"), 16, std::to_string(disk_shard));
				}
				return data_path + "/ht_" + m_db_name + "_" + std::to_string(m_shard_id);
			}

			std::string file_base() const {
				const size_t disk_shard = m_shard_id % 8;
				std::string data_path = "/mnt/{shard_id_mod_8}/hash_table";
				if (data_path.find("{shard_id_mod_8}") != std::string::npos) {
					data_path.replace(data_path.find("{shard_id_mod_8}"), 16, std::to_string(disk_shard));
				}
				return data_path + "/ht_" + m_db_name + "_" + std::to_string(m_shard_id);
			}

			std::string filename_data() const {
				return file_base_data() + ".data";
			}

			std::string filename_pos() const {
				return file_base() + ".pos";
			}

			std::string filename_data_tmp() const {
				return file_base() + ".data.tmp";
			}

		protected:

			const std::string m_db_name;
			size_t m_shard_id;
			size_t m_hash_table_size;
			const std::string m_data_path;

			size_t hash_table_byte_size() const { return m_hash_table_size * sizeof(size_t); }

			std::vector<std::vector<std::array<uint64_t, 3>>> read_pages() const {
				std::ifstream infile(filename_pos(), std::ios::binary);
				return read_pages(infile);
			}

			std::vector<std::vector<std::array<uint64_t, 3>>> read_pages(std::ifstream &infile) const {
				
				const size_t max_records = 10000;
				const size_t record_len = sizeof(std::array<uint64_t, 3>);
				const size_t buffer_len = record_len * max_records;

				auto buffer_allocator = std::make_unique<char[]>(buffer_len);
				char *buffer = buffer_allocator.get();

				std::vector<std::vector<std::array<uint64_t, 3>>> ret(this->m_hash_table_size);

				if (infile.is_open()) {
					infile.seekg(this->hash_table_byte_size());

					do {
						size_t num_keys;
						infile.read((char *)&num_keys, sizeof(size_t));

						if (infile.eof()) break;

						if (num_keys > max_records) {
							break;
						}

						const size_t len = record_len * num_keys;
						infile.read(buffer, len);

						for (size_t i = 0; i < len; i += record_len) {
							const uint64_t key = *((uint64_t *)&buffer[i]);
							const size_t page_id = key % this->m_hash_table_size;
							const size_t pos = *((size_t *)&buffer[i + sizeof(uint64_t)]);
							const size_t version = *((size_t *)&buffer[i + sizeof(uint64_t) + sizeof(size_t)]);
							ret[page_id].emplace_back(std::array{key, (uint64_t)pos, (uint64_t)version});
						}

					} while (!infile.eof());
				}
			
				return ret;
			}

	};

}
