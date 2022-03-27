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

#include "parser/URL.h"
#include "system/SubSystem.h"
#include "hash_table/HashTableShardBuilder.h"
#include "full_text/url_to_domain.h"
#include "full_text/full_text_shard_builder.h"
#include "url_link/full_text_record.h"

namespace url_link {

	class indexer {

	public:

		indexer(int id, const std::string &db_name, const SubSystem *sub_system, full_text::url_to_domain *url_to_domain);
		~indexer();

		void add_stream(std::vector<HashTableShardBuilder *> &shard_builders, std::basic_istream<char> &stream);
		void write_cache(std::vector<std::mutex> &write_mutexes);
		void flush_cache(std::vector<std::mutex> &write_mutexes);

	private:

		int m_indexer_id;
		const std::string m_db_name;
		const SubSystem *m_sub_system;
		full_text::url_to_domain *m_url_to_domain;
		std::hash<std::string> m_hasher;

		std::vector<full_text::full_text_shard_builder<::url_link::full_text_record> *> m_shards;

		void add_expanded_data_to_shards(uint64_t link_hash, const URL &source_url, const URL &target_url, const std::string &link_text,
			float score);

	};

}
