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
#include "index_builder.h"
#include "index.h"
#include "sharded_index_builder.h"
#include "sharded_index.h"
#include "level.h"
#include "snippet.h"
#include "hash_table2/builder.h"
#include "sharded_builder.h"
#include "sharded.h"
#include "counted_index_builder.h"
#include "counted_index.h"
#include "counted_record.h"
#include "domain_record.h"
#include "algorithm/bloom_filter.h"

namespace indexer {

	class index_manager {

	public:

		index_manager();
		index_manager(bool only_links);
		index_manager(bool only_links, bool only_snippets);
		~index_manager();

		void add_level(level *lvl);
		void add_snippet(const snippet &s);
		void add_document(size_t id, const std::string &doc);
		void add_index_file(const std::string &local_path);
		void add_index_files_threaded(const vector<string> &local_paths, size_t num_threads);
		void add_link_file(const std::string &local_path, const ::algorithm::bloom_filter &urls_to_index);
		void add_link_files_threaded(const std::vector<std::string> &local_paths, size_t num_threads, const ::algorithm::bloom_filter &urls_to_index);
		void add_url_link_file(const std::string &local_path, const ::algorithm::bloom_filter &urls_to_index);
		void add_url_link_files_threaded(const std::vector<std::string> &local_paths, size_t num_threads, const ::algorithm::bloom_filter &urls_to_index);
		void add_title_file(const std::string &local_path);
		void add_title_files_threaded(const std::vector<std::string> &local_paths, size_t num_threads);
		void add_link_count_file(const std::string &local_path);
		void add_link_count_files_threaded(const std::vector<std::string> &local_paths, size_t num_threads);
		void add_url_file(const std::string &local_path);
		void add_url_files_threaded(const std::vector<std::string> &local_paths, size_t num_threads);
		void add_snippet_file(const std::string &local_path);
		void add_snippet_files_threaded(const std::vector<std::string> &local_paths, size_t num_threads);
		void add_word_file(const std::string &local_path, const std::set<uint64_t> &common_words);
		void add_word_files_threaded(const std::vector<std::string> &local_paths, size_t num_threads, const std::set<uint64_t> &words_to_index);
		void merge();
		void optimize();
		void merge_word();
		void truncate();
		void truncate_links();
		void truncate_words();
		void clean_up();
		void calculate_scores_for_level(size_t level_num);

		std::vector<return_record> find(size_t &total_num_results, const std::string &query);
		std::vector<return_record> find(const std::string &query);

	private:

		std::unique_ptr<sharded_builder<counted_index_builder, link_record>> m_link_index_builder;
		std::unique_ptr<sharded<counted_index, link_record>> m_link_index;

		std::unique_ptr<sharded_builder<counted_index_builder, domain_link_record>> m_domain_link_index_builder;
		std::unique_ptr<sharded<counted_index, domain_link_record>> m_domain_link_index;

		std::unique_ptr<sharded_builder<counted_index_builder, counted_record>> m_word_index_builder;
		std::unique_ptr<sharded<counted_index, counted_record>> m_word_index;

		std::unique_ptr<sharded_builder<counted_index_builder, counted_record>> m_fp_title_word_builder;

		std::unique_ptr<sharded_builder<counted_index_builder, counted_record>> m_title_word_builder;
		std::unique_ptr<sharded<counted_index, counted_record>> m_title_word_counter;

		std::unique_ptr<sharded_builder<counted_index_builder, counted_record>> m_link_word_builder;
		std::unique_ptr<sharded<counted_index, counted_record>> m_link_word_counter;

		std::unique_ptr<sharded_index<domain_record>> m_domain_info;

		std::vector<level *> m_levels;
		std::unique_ptr<hash_table2::builder> m_hash_table;
		std::unique_ptr<hash_table2::builder> m_hash_table_words;
		std::unique_ptr<hash_table2::builder> m_hash_table_snippets;
		//std::unique_ptr<full_text::url_to_domain> m_url_to_domain;

		void create_directories(level_type lvl);
		void delete_directories(level_type lvl);

	};

}
