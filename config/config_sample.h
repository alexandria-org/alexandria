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
	inline const unsigned long long nodes_in_cluster = 3;
	inline const unsigned long long node_id = 0;

	inline const std::vector<std::string> batches = {
		"ALEXANDRIA-MANUAL-01",
		"CC-MAIN-2021-25",
		"CC-MAIN-2021-31",
	};

	inline const std::vector<std::string> link_batches = {
		"CC-MAIN-2021-31",
		"CC-MAIN-2021-25",
		"CC-MAIN-2021-21",
		"CC-MAIN-2021-17",
		"CC-MAIN-2021-10",
		"CC-MAIN-2021-04",
		"CC-MAIN-2020-50",
		"CC-MAIN-2020-45",
	};

	// Full text indexer config
	inline const unsigned long long ft_num_shards = 1024;
	inline const unsigned long long ft_num_partitions = 8;
	inline const unsigned long long ft_num_link_partitions = 8;
	inline const unsigned long long ft_max_keys = 0xFFFFFFFF;
	inline const unsigned long long ft_max_cache_gb = 30;
	inline const unsigned long long ft_num_threads_indexing = 24;
	inline const unsigned long long ft_num_threads_merging = 24;
	inline const double ft_cached_bytes_per_shard  = (ft_max_cache_gb * 1000ul*1000ul*1000ul) / (ft_num_shards * ft_num_threads_indexing);

	// Link indexer config
	inline const unsigned long long li_max_cache_gb = 4;
	inline const unsigned long long li_num_threads_indexing = 48;
	inline const unsigned long long li_num_threads_merging = 16;
	inline const double li_cached_bytes_per_shard  = (li_max_cache_gb * 1000ul*1000ul*1000ul) / (ft_num_shards * li_num_threads_indexing);
	inline const unsigned long long li_indexer_max_cache_size = 500;

	// Hash table indexer config
	inline const unsigned long long ht_num_shards = 1024;
	inline const unsigned long long ht_num_buckets = 8;
	inline const unsigned long long ht_key_size = 8;

	// Server config
	inline const unsigned int worker_count = 8;

	// Other constants.
	inline const unsigned long long num_async_file_transfers = 48;
	inline const std::string test_data_path = "/var/www/html/node0003.alexandria.org/test-data/";

	// Commoncrawl parser.
	inline const std::string cc_target_output = "alexandria-cc-output";
	inline const bool cc_run_on_lambda = true;

}

