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
#include <vector>
#include <functional>

#include "config.h"
#include "hash_table_shard_base.h"

namespace hash_table2 {

	class hash_table_shard : public hash_table_shard_base {

		public:

			hash_table_shard(const std::string &db_name, size_t shard_id, size_t hash_table_size = 1000000,
					const std::string &data_path = config::data_path() + "/{shard_id_mod_8}/hash_table");
			~hash_table_shard();

			/*
			 * Checks if the key exists in the hash table.
			 * */
			bool has(uint64_t key) const;

			/*
			 * Finds a value for the given key. Returns empty string if key is not present.
			 * */
			std::string find(uint64_t key) const;

			/*
			 * Finds a value for the given key. Returns empty string if key is not present. Also sets version in 'ver'
			 * */
			std::string find(uint64_t key, size_t &ver) const;

			/*
			 * Applies the given function to all elements in hash table shard. 
			 * */
			void for_each(std::function<void(uint64_t, std::string)>) const;

			/*
			 * Returns the id of the shard.
			 * */
			size_t shard_id() const;

			/*
			 * Returns the number of elements in the shard.
			 * */
			size_t size() const;

			/*
			 * Returns the size of the data file in bytes.
			 * */
			size_t file_size() const;

		private:

			std::string data_at_position(size_t pos) const;

	};

}
