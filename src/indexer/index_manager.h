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

#include <memory>
#include "full_text/search_metric.h"
#include "index_builder.h"
#include "index.h"
#include "sharded_index_builder.h"
#include "sharded_index.h"
#include "hash_table2/builder.h"
#include "sharded_builder.h"
#include "sharded.h"
#include "basic_index_builder.h"
#include "basic_index.h"
#include "counted_record.h"
#include "url_record.h"
#include "link_record.h"
#include "domain_link_record.h"
#include "domain_record.h"
#include "return_record.h"
#include "algorithm/bloom_filter.h"

namespace indexer {

	class index_manager {

	public:

		index_manager();
		~index_manager();

		void add_index_file(const std::string &local_path);
		void add_index_files_threaded(const vector<string> &local_paths, size_t num_threads);
		void add_link_file(const std::string &local_path, const ::algorithm::bloom_filter &urls_to_index);
		void add_link_files_threaded(const std::vector<std::string> &local_paths, size_t num_threads, const ::algorithm::bloom_filter &urls_to_index);
		void add_url_file(const std::string &local_path);
		void add_url_files_threaded(const std::vector<std::string> &local_paths, size_t num_threads);

		void merge();
		void optimize();
		void truncate();
		void truncate_links();

		size_t url_count() const {
			return m_hash_table->size();
		}

		std::vector<return_record> find(const std::string &query, full_text::search_metric &metric);
		std::vector<return_record> find(const std::string &query);

	private:

		std::unique_ptr<sharded_builder<basic_index_builder, url_record>> m_url_index_builder;
		std::unique_ptr<sharded<basic_index, url_record>> m_url_index;

		std::unique_ptr<sharded_builder<basic_index_builder, link_record>> m_link_index_builder;
		std::unique_ptr<sharded<basic_index, link_record>> m_link_index;

		std::unique_ptr<sharded_builder<basic_index_builder, domain_link_record>> m_domain_link_index_builder;
		std::unique_ptr<sharded<basic_index, domain_link_record>> m_domain_link_index;

		std::unique_ptr<hash_table2::builder> m_hash_table_builder;
		std::unique_ptr<hash_table2::hash_table> m_hash_table;

		size_t apply_domain_link_scores(const vector<domain_link_record> &links, std::vector<url_record> &results);
		size_t apply_link_scores(const vector<link_record> &links, std::vector<url_record> &results);
		std::vector<return_record> decorate_search_result(const std::vector<url_record> &results);
		std::vector<url_record> deduplicate_search_results(const std::vector<url_record> &results, size_t limit);

	};

}
