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

#include <vector>
#include <string>

namespace Config {

	// Cluster config
	// Not const since we need to modify them for tests.
	extern unsigned long long nodes_in_cluster;
	extern unsigned long long node_id;

	extern std::vector<std::string> batches;
	extern std::vector<std::string> link_batches;

	// Full text indexer config
	const unsigned long long ft_num_shards = 1024;
	const unsigned long long ft_num_partitions = 4;
	const unsigned long long ft_num_link_partitions = 4;
	const unsigned long long ft_max_results_per_partition = 100000;
	const unsigned long long ft_max_keys = 0xFFFFFFFF;
	const unsigned long long ft_max_cache_gb = 30;
	const unsigned long long ft_num_threads_indexing = 24;
	const unsigned long long ft_num_threads_merging = 24;
	const double ft_cached_bytes_per_shard  = (ft_max_cache_gb * 1000ul*1000ul*1000ul) / (ft_num_shards * ft_num_threads_indexing);

	// Link indexer config
	const unsigned long long li_max_cache_gb = 4;
	const unsigned long long li_num_threads_indexing = 48;
	const unsigned long long li_num_threads_merging = 16;
	const double li_cached_bytes_per_shard  = (li_max_cache_gb * 1000ul*1000ul*1000ul) / (ft_num_shards * li_num_threads_indexing);
	const unsigned long long li_indexer_max_cache_size = 500;

	// Hash table indexer config
	const unsigned long long ht_num_shards = 16384;
	const unsigned long long ht_num_buckets = 8;
	const unsigned long long ht_key_size = 8;

	// Server config
	const unsigned int worker_count = 8;
	const unsigned long long query_max_words = 10; // Maximum number of words used in query.

	// Other constants.
	const unsigned long long num_async_file_transfers = 48;
	const std::string test_data_path = "/var/www/html/node0003.alexandria.org/test-data/";

	// Commoncrawl parser.
	const std::string cc_target_output = "alexandria-cc-output";
	const bool cc_run_on_lambda = true;

}

