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
#include "HashTableHelper.h"
#include "system/Logger.h"

namespace HashTableHelper {

	void truncate(const string &hash_table_name) {
		vector<HashTableShardBuilder *> shards = create_shard_builders(hash_table_name);

		for (HashTableShardBuilder *shard : shards) {
			shard->truncate();
		}

		delete_shard_builders(shards);
	}

	vector<HashTableShardBuilder *> create_shard_builders(const string &hash_table_name) {
		vector<HashTableShardBuilder *> shards;
		for (size_t shard_id = 0; shard_id < Config::ht_num_shards; shard_id++) {
			shards.push_back(new HashTableShardBuilder(hash_table_name, shard_id));
		}

		return shards;
	}

	void delete_shard_builders(vector<HashTableShardBuilder *> &shards) {
		for (HashTableShardBuilder *shard : shards) {
			delete shard;
		}

		shards.clear();
	}

	void add_data(vector<HashTableShardBuilder *> &shards, uint64_t key, const string &value) {
		shards[key % Config::ht_num_shards]->add(key, value);
	}

	void write(vector<HashTableShardBuilder *> &shards) {
		for (HashTableShardBuilder *shard : shards) {
			shard->write();
		}
	}

	void sort(vector<HashTableShardBuilder *> &shards) {
		for (HashTableShardBuilder *shard : shards) {
			shard->sort();
		}
	}

	void optimize(vector<HashTableShardBuilder *> &shards) {
		for (HashTableShardBuilder *shard : shards) {
			LogInfo("Optimizing shard: " + shard->filename_data());
			shard->optimize();
		}
	}

}
