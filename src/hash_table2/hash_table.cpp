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

namespace hash_table2 {

	hash_table::hash_table(const std::string &db_name, size_t num_shards, size_t hash_table_size, const std::string &data_path)
	: m_db_name(db_name)
	{
		for (size_t shard_id = 0; shard_id < num_shards; shard_id++) {
			auto shard = new hash_table_shard(m_db_name, shard_id, hash_table_size, data_path);
			m_shards.push_back(shard);
		}
	}

	hash_table::~hash_table() {
		for (hash_table_shard *shard : m_shards) {
			delete shard;
		}
	}

	void hash_table::add(uint64_t key, const std::string &value) {

		const size_t shard_id = key % m_shards.size();
		hash_table_shard_builder builder(m_db_name, shard_id);

		builder.add(key, value);
	}

	void hash_table::truncate() {
		for (size_t shard_id = 0; shard_id < m_shards.size(); shard_id++) {
			hash_table_shard_builder builder(m_db_name, shard_id);
			builder.truncate();
		}
	}

	bool hash_table::has(uint64_t key) {
		return m_shards[key % m_shards.size()]->has(key);
	}

	std::string hash_table::find(uint64_t key) {
		size_t ver = 0;
		return find(key, ver);
	}

	std::string hash_table::find(uint64_t key, size_t &ver) {
		return m_shards[key % m_shards.size()]->find(key, ver);
	}

	size_t hash_table::size() const {
		size_t num_items = 0;
		for (const auto &shard : m_shards) {
			num_items += shard->size();
		} 
		return num_items;
	}

	void hash_table::for_each(std::function<void(uint64_t, const std::string &)> callback) const {
		for (const auto &shard : m_shards) {
			shard->for_each(callback);
		}
	}

	void hash_table::for_each_key(std::function<void(uint64_t)> callback) const {
		for (const auto &shard : m_shards) {
			shard->for_each_key(callback);
		}
	}

	void hash_table::for_each_shard(std::function<void(const hash_table_shard *shard)> callback) const {
		for (const auto &shard : m_shards) {
			callback(shard);
		}
	}

}
