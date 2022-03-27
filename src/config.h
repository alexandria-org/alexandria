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
#include <fstream>
#include <vector>

namespace config {

	extern std::string node;
	extern std::string master;
	extern std::string upload;
	extern std::string data_node;
	extern std::string url_store_host;
	extern std::string url_store_path;
	extern std::string url_store_cache_path;

	const size_t url_store_shards = 24;

	extern size_t nodes_in_cluster;
	extern size_t node_id;

	extern bool index_snippets;
	extern bool index_text;

	extern std::vector<std::string> batches;
	extern std::vector<std::string> link_batches;

	extern size_t worker_count;
	extern size_t query_max_words;
	extern size_t query_max_len;
	extern size_t deduplicate_domain_count;
	extern size_t pre_result_limit;
	extern size_t result_limit;
	extern std::string file_upload_user;
	extern std::string file_upload_password;
	extern size_t n_grams;
	extern size_t shard_hash_table_size;
	extern size_t html_parser_long_text_len;
	extern size_t ft_shard_builder_buffer_len;

	/*
		Constants only configurable at compilation time.
	*/

	// Full text indexer config
	extern size_t ft_num_shards;
	extern size_t ft_max_sections;
	extern size_t ft_max_results_per_section;
	extern size_t ft_section_depth;
	extern size_t ft_max_cache_gb;
	extern size_t ft_num_threads_indexing;
	extern size_t ft_num_threads_merging;
	extern size_t ft_num_threads_appending;
	double ft_cached_bytes_per_shard();

	// Link indexer config
	inline const unsigned long long li_max_cache_gb = 4;
	inline const unsigned long long li_num_threads_indexing = 48;
	inline const unsigned long long li_num_threads_merging = 16;
	inline const double li_cached_bytes_per_shard  = (li_max_cache_gb * 1000ul*1000ul*1000ul) / (ft_num_shards * li_num_threads_indexing);
	inline const unsigned long long li_indexer_max_cache_size = 500;

	// Hash table indexer config
	inline const unsigned long long ht_num_shards = 1031;
	inline const unsigned long long ht_num_buckets = 8;
	inline const unsigned long long ht_key_size = 8;

	// Server config

	// Other constants.
	inline const unsigned long long num_async_file_transfers = 48;
	inline const std::string test_data_path = "/var/www/html/node0003.alexandria.org/test-data/";

	// Commoncrawl parser.
	inline const std::string cc_target_output = "alexandria-cc-output";
	inline const bool cc_run_on_lambda = false;

	inline const std::string log_file_path = "/var/log/alexandria.log";

	void read_config(const std::string &config_file);

}


