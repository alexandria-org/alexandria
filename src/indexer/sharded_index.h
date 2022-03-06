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

#include "index.h"
#include "algorithm/intersection.h"

namespace indexer {

	template<typename data_record>
	class sharded_index {

	public:

		sharded_index(const std::string &db_name, size_t num_shards);
		sharded_index(const std::string &db_name, size_t num_shards, size_t hash_table_size);
		~sharded_index();

		std::vector<data_record> find(uint64_t key) const;
		std::vector<data_record> find(const std::vector<uint64_t> &keys) const;

	private:

		std::vector<std::shared_ptr<index<data_record>>> m_shards;
		
	};

	template<typename data_record>
	sharded_index<data_record>::sharded_index(const std::string &db_name, size_t num_shards) {
		for (size_t shard_id = 0; shard_id < num_shards; shard_id++) {
			m_shards.push_back(std::make_shared<index<data_record>>(db_name, shard_id));
		}
	}

	template<typename data_record>
	sharded_index<data_record>::sharded_index(const std::string &db_name, size_t num_shards, size_t hash_table_size) {
		for (size_t shard_id = 0; shard_id < num_shards; shard_id++) {
			m_shards.push_back(std::make_shared<index<data_record>>(db_name, shard_id, hash_table_size));
		}
	}

	template<typename data_record>
	sharded_index<data_record>::~sharded_index() {
	}

	template<typename data_record>
	std::vector<data_record> sharded_index<data_record>::find(uint64_t key) const {
		return m_shards[key % m_shards.size()]->find(key);
	}

	template<typename data_record>
	std::vector<data_record> sharded_index<data_record>::find(const std::vector<uint64_t> &keys) const {

		std::vector<std::vector<data_record>> results;
		for (uint64_t key : keys) {
			results.emplace_back(find(key));
		}

		return ::algorithm::intersection(results);
	}

}
