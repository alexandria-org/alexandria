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
#include <thread>
#include <vector>
#include <map>

#include "config.h"
#include "hash_table_shard.h"

namespace hash_table2 {

	class hash_table_shard;

	class hash_table {

	public:

		explicit hash_table(const std::string &db_name, size_t num_shards = config::ht_num_shards,
				size_t hash_table_size = 1000000,
				const std::string &data_path = config::data_path() + "/{shard_id_mod_8}/hash_table");
		~hash_table();

		void add(uint64_t key, const std::string &value);
		void truncate();
		bool has(uint64_t key);
		std::string find(uint64_t key);
		std::string find(uint64_t key, size_t &ver);
		size_t size() const;
		void for_each(std::function<void(uint64_t, const std::string &)> callback) const;
		void for_each_shard(std::function<void(const hash_table_shard *shard)> callback) const;

	private:

		std::vector<hash_table_shard *> m_shards;
		const std::string m_db_name;

	};

}
