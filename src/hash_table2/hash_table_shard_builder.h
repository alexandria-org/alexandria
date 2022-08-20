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
#include <map>
#include <mutex>

#include "hash_table.h"
#include "hash_table_shard_base.h"

namespace hash_table2 {

	/*
	 * Implementation of a hash table shard.
	 *
	 * usage:
	 * hash_table_shard shard("test_db", 0);
	 * shard.add(12345, "test data", 3);
	 * shard.add(12345, "new test data", 4);
	 *
	 * shard.append();
	 * shard.merge();
	 *
	 * */

	class hash_table_shard_builder : public hash_table_shard_base {

		public:

			hash_table_shard_builder(const std::string &db_name, size_t shard_id, size_t hash_table_size = 1000000,
					const std::string &data_path = config::data_path() + "/{shard_id_mod_8}/hash_table");
			~hash_table_shard_builder();

			/*
			 * Add key/value pair to hash table.
			 * */
			void add(uint64_t key, const std::string &value, size_t version = 0);

			/*
			 * Remove key from hash table.
			 * */
			void remove(uint64_t key);

			/*
			 * Return approximation of amount of memory in cache.
			 * */
			size_t cache_size() const;

			/*
			 * Write memory cache to disc cache.
			 * */
			void append();

			/*
			 * Write disc cache to persistant hash table.
			 * */
			void merge();

			/*
			 * Optimize persistant has table to remove data for unused versions.
			 * */
			void optimize();

			/*
			 * Delete all data in shard.
			 * */
			void truncate();

			/*
			 * Merge with another shard. Handles key collisions by keeping the one with highest version.
			 * */
			void merge_with(const hash_table_shard_builder &other);

			/*
			 * Merge with another pos and datafile.
			 * */
			void merge_with(const std::string &pos_file, const std::string &data_file);

		private:

			std::map<uint64_t, std::string> m_cache;
			std::map<uint64_t, size_t> m_version;
			std::vector<uint64_t> m_remove_keys;

			std::map<uint64_t, size_t> m_sort_pos;
			std::mutex m_lock;
			size_t m_data_size = 0;

			void read_optimized_to(const std::vector<std::vector<std::array<uint64_t, 3>>> &pages, std::ifstream &infile, std::ofstream &outfile) const;
			void write_pages(const std::vector<std::vector<std::array<uint64_t, 3>>> &pages);
			void remove_keys_from_pages(std::vector<std::vector<std::array<uint64_t, 3>>> &pages);

	};

}
