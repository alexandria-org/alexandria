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

#include "config.h"
#include "hash_table_helper.h"
#include "logger/logger.h"

using namespace std;

namespace hash_table_helper {

	void truncate(const string &hash_table_name) {
		vector<hash_table2::hash_table_shard_builder *> shards = create_shard_builders(hash_table_name);

		for (auto shard : shards) {
			shard->truncate();
		}

		delete_shard_builders(shards);
	}

	vector<hash_table2::hash_table_shard_builder *> create_shard_builders(const string &hash_table_name) {
		vector<hash_table2::hash_table_shard_builder *> shards;
		for (size_t shard_id = 0; shard_id < config::ht_num_shards; shard_id++) {
			shards.push_back(new hash_table2::hash_table_shard_builder(hash_table_name, shard_id));
		}

		return shards;
	}

	void delete_shard_builders(vector<hash_table2::hash_table_shard_builder *> &shards) {
		for (auto shard : shards) {
			delete shard;
		}

		shards.clear();
	}

}
