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

#include "builder.h"
#include "config.h"
#include "utils/thread_pool.hpp"

using namespace std;

namespace hash_table {

	builder::builder(const string &db_name)
	: m_db_name(db_name) {
		for (size_t i = 0; i < Config::ht_num_shards; i++) {
			m_shards.push_back(new HashTableShardBuilder(db_name, i));
		}
	}

	builder::~builder() {
		for (HashTableShardBuilder *shard : m_shards) {
			delete shard;
		}
	}

	void builder::add(uint64_t key, const std::string &value) {
		m_shards[key % Config::ht_num_shards]->add(key, value);
	}

	void builder::merge() {
		cout << "Merging hash table" << endl;
		utils::thread_pool pool(24);
		for (HashTableShardBuilder *shard : m_shards) {
			pool.enqueue([shard]() -> void {
				shard->write();
				shard->sort();
				shard->optimize();
			});
		}

		pool.run_all();

		cout << "...done" << endl;
	}
}
