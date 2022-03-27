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
#include <map>
#include <cstdint>
#include "url_link/link.h"
#include "parser/URL.h"
#include "text/text.h"
#include "url_to_domain.h"
#include "full_text_record.h"
#include "full_text_index.h"
#include "worker/worker.h"

namespace full_text {

	using std::string;
	using std::vector;

	size_t num_shards();

	void truncate_url_to_domain(const string &index_name);
	void truncate_index(const string &index_name);

	std::map<uint64_t, float> tsv_data_to_scores(const string &tsv_data, const SubSystem *sub_system);
	void add_words_to_word_map(const vector<string> &words, float score, std::map<uint64_t, float> &word_map);

	vector<string> download_batch(const string &batch, size_t limit, size_t offset);
	bool is_indexed();
	size_t total_urls_in_batches();

	void index_batch(const string &db_name, const string &hash_table_name, const string &batch, const SubSystem *sub_system);
	void index_batch(const string &db_name, const string &hash_table_name, const string &batch, const SubSystem *sub_system, worker::status &status);
	void index_link_batch(const string &db_name, const string &domain_db_name, const string &hash_table_name, const string &domain_hash_table_name,
		const string &batch, const SubSystem *sub_system, url_to_domain *url_to_domain);
	void index_link_batch(const string &db_name, const string &domain_db_name, const string &hash_table_name, const string &domain_hash_table_name,
		const string &batch, const SubSystem *sub_system, url_to_domain *url_to_domain, worker::status &status);

	void index_all_batches(const string &db_name, const string &hash_table_name);
	void index_all_batches(const string &db_name, const string &hash_table_name, worker::status &status);

	void index_single_batch(const string &db_name, const string &domain_db_name, const string &batch);

	void index_all_link_batches(const string &db_name, const string &domain_db_name, const string &hash_table_name,
			const string &domain_hash_table_name);
	void index_all_link_batches(const string &db_name, const string &domain_db_name, const string &hash_table_name,
			const string &domain_hash_table_name, worker::status &status);

	void index_link_batch(const string &db_name, const string &domain_db_name, const string &hash_table_name, const string &domain_hash_table_name,
		const string &batch, const SubSystem *sub_system, url_to_domain *url_to_domain);
	void index_link_batch(const string &db_name, const string &domain_db_name, const string &hash_table_name, const string &domain_hash_table_name,
		const string &batch, const SubSystem *sub_system, url_to_domain *url_to_domain, worker::status &status);

	void index_single_link_batch(const string &db_name, const string &domain_db_name, const string &hash_table_name,
		const string &domain_hash_table_name, const string &batch, url_to_domain *url_to_domain);

	size_t url_to_node(const URL &url);
	bool should_index_url(const URL &url);

	size_t link_to_node(const url_link::link &link);
	bool should_index_link(const url_link::link &link);

}
