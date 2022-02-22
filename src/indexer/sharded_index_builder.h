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

#include "index_builder.h"

namespace indexer {

	template<typename data_record>
	class sharded_index_builder {
	private:
		// Non copyable
		sharded_index_builder(const sharded_index_builder &);
		sharded_index_builder& operator=(const sharded_index_builder &);
	public:

		sharded_index_builder(const std::string &db_name, size_t num_shards);
		sharded_index_builder(const std::string &db_name, size_t num_shards, size_t hash_table_size);
		~sharded_index_builder();

		void add(uint64_t key, const data_record &record);
		
		void append();
		void merge();

		void truncate();
		void truncate_cache_files();
		void create_directories();

	private:

		std::vector<std::shared_ptr<index_builder<data_record>>> m_shards;

	};

	template<typename data_record>
	sharded_index_builder<data_record>::sharded_index_builder(const std::string &db_name, size_t num_shards) {
		for (size_t shard_id = 0; shard_id < num_shards; shard_id++) {
			m_shards.push_back(std::make_shared<index_builder<data_record>>(db_name, shard_id));
		}
	}

	template<typename data_record>
	sharded_index_builder<data_record>::sharded_index_builder(const std::string &db_name, size_t num_shards, size_t hash_table_size) {
		for (size_t shard_id = 0; shard_id < num_shards; shard_id++) {
			m_shards.push_back(std::make_shared<index_builder<data_record>>(db_name, shard_id, hash_table_size));
		}
	}

	template<typename data_record>
	sharded_index_builder<data_record>::~sharded_index_builder() {
	}

	template<typename data_record>
	void sharded_index_builder<data_record>::add(uint64_t key, const data_record &record) {
		m_shards[key % m_shards.size()]->add(key, record);
	}

	template<typename data_record>
	void sharded_index_builder<data_record>::append() {
		for (auto &shard : m_shards) {
			shard->append();
		}
	}

	template<typename data_record>
	void sharded_index_builder<data_record>::merge() {
		for (auto &shard : m_shards) {
			shard->merge();
		}
	}

	template<typename data_record>
	void sharded_index_builder<data_record>::truncate() {
		for (auto &shard : m_shards) {
			shard->truncate();
		}
	}

	template<typename data_record>
	void sharded_index_builder<data_record>::truncate_cache_files() {
		for (auto &shard : m_shards) {
			shard->truncate_cache_files();
		}
	}

	template<typename data_record>
	void sharded_index_builder<data_record>::create_directories() {
		for (auto &shard : m_shards) {
			shard->create_directories();
		}
	}


}
