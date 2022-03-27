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
#include <istream>
#include <vector>
#include <mutex>
#include <unordered_map>

class full_text_indexer;

#include "full_text_shard.h"
#include "full_text_shard_builder.h"
#include "full_text_index.h"
#include "url_to_domain.h"
#include "parser/URL.h"
#include "system/SubSystem.h"
#include "hash_table/HashTableShardBuilder.h"
#include "url_link/link.h"
#include "full_text_record.h"

namespace full_text {

	class full_text_indexer {

		public:

			full_text_indexer(int id, const std::string &db_name, const SubSystem *sub_system, url_to_domain *url_to_domain);
			~full_text_indexer();

			size_t add_stream(std::vector<HashTableShardBuilder *> &shard_builders, std::basic_istream<char> &stream,
				const std::vector<size_t> &cols, const std::vector<float> &scores, const std::string &batch, std::mutex &write_mutex);
			size_t write_cache(std::mutex &write_mutex);
			size_t write_large(std::vector<std::mutex> &write_mutexes);
			void flush_cache(std::vector<std::mutex> &write_mutexes);
			void read_url_to_domain();
			void write_url_to_domain();
			void add_domain_link(uint64_t word_hash, const url_link::link &link);
			void add_url_link(uint64_t word_hash, const url_link::link &link);

			bool has_key(uint64_t key) const {
				return m_url_to_domain->get_url_to_domain().count(key) > 0;
			}

			bool has_domain(uint64_t domain_hash) const {
				auto iter = m_url_to_domain->domains().find(domain_hash);
				if (iter == m_url_to_domain->domains().end()) return false;
				return iter->second > 0;
			}

			const url_to_domain *get_url_to_domain() const {
				return m_url_to_domain;
			}

		private:

			int m_indexer_id;
			const std::string m_db_name;
			const SubSystem *m_sub_system;
			std::hash<std::string> m_hasher;
			std::vector<full_text_shard_builder<struct full_text_record> *> m_shards;

			url_to_domain *m_url_to_domain = NULL;

			void add_expanded_data_to_word_map(std::map<uint64_t, float> &word_map, const std::string &text, float score) const;
			void add_data_to_word_map(std::map<uint64_t, float> &word_map, const std::string &text, float score) const;
			void add_data_to_shards(const URL &url, const std::string &text, float score);

	};

}
