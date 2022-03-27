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
#include "hash_table.h"
#include "hash_table_shard_builder.h"
#include "logger/logger.h"

using namespace std;

namespace hash_table {

	hash_table::hash_table(const string &db_name)
	: m_db_name(db_name)
	{
		m_num_items = 0;
		for (size_t shard_id = 0; shard_id < config::ht_num_shards; shard_id++) {
			auto shard = new hash_table_shard(m_db_name, shard_id);
			m_num_items += shard->size();
			m_shards.push_back(shard);
		}

		LOG_INFO("hash_table contains " + to_string(m_num_items) + " (" + to_string((double)m_num_items/1000000000) + "b) urls");
	}

	hash_table::~hash_table() {

		for (hash_table_shard *shard : m_shards) {
			delete shard;
		}
	}

	void hash_table::add(uint64_t key, const string &value) {

		const size_t shard_id = key % config::ht_num_shards;
		hash_table_shard_builder builder(m_db_name, shard_id);

		builder.add(key, value);

		builder.write();

	}

	void hash_table::truncate() {
		if (m_num_items > 1000000) {
			cout << "Are you sure you want to truncate hash table containing " << m_num_items << " elements? [y/N]: ";
			string line;
			cin >> line;
			if (line != "y") {
				cout << "Exiting..." << endl;
				exit(0);
			}
		}
		for (size_t shard_id = 0; shard_id < config::ht_num_shards; shard_id++) {
			hash_table_shard_builder builder(m_db_name, shard_id);
			builder.truncate();
		}
	}

	string hash_table::find(uint64_t key) {
		return m_shards[key % config::ht_num_shards]->find(key);
	}

	size_t hash_table::size() const {
		return m_num_items;
	}

	void hash_table::print_all_items() const {
		for (hash_table_shard *shard : m_shards) {
			shard->print_all_items();
		}
	}

}
