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
#include "utils/thread_pool.hpp"

using namespace std;

namespace hash_table2 {

	builder::builder(const string &db_name, size_t num_shards, size_t hash_table_size,
			const std::string &data_path)
	: m_db_name(db_name) {
		for (size_t i = 0; i < num_shards; i++) {
			m_shards.push_back(new hash_table_shard_builder(db_name, i, hash_table_size, data_path));
		}
	}

	builder::~builder() {
		for (hash_table_shard_builder *shard : m_shards) {
			delete shard;
		}
	}

	void builder::add(uint64_t key, const std::string &value, size_t version) {
		m_shards[key % m_shards.size()]->add(key, value, version);
	}

	void builder::remove(uint64_t key) {
		m_shards[key % m_shards.size()]->remove(key);
	}

	void builder::merge() {
		utils::thread_pool pool(32);
		for (hash_table_shard_builder *shard : m_shards) {
			pool.enqueue([shard]() -> void {
				shard->append();
				shard->merge();
			});
		}

		pool.run_all();
	}

	void builder::optimize() {
		utils::thread_pool pool(32);
		for (hash_table_shard_builder *shard : m_shards) {
			pool.enqueue([shard]() -> void {
				shard->optimize();
			});
		}

		pool.run_all();
	}

	void builder::truncate() {
		for (hash_table_shard_builder *shard : m_shards) {
			shard->truncate();
		}
	}

	void builder::merge_with(const builder &other) {
		for (size_t i = 0; i < m_shards.size(); i++) {
			m_shards[i]->merge_with(*(other.m_shards[i]));
		}
	}
}
