
#pragma once

#include <iostream>
#include <fstream>
#include <vector>

namespace Config {

	extern size_t nodes_in_cluster;
	extern size_t node_id;
	extern std::vector<std::string> batches;
	extern std::vector<std::string> link_batches;
	extern size_t worker_count;
	extern size_t query_max_words;
	extern size_t query_max_len;
	extern size_t deduplicate_domain_count;
	extern size_t pre_result_limit;
	extern size_t result_limit;
	extern size_t ft_max_sections;
	extern size_t ft_max_results_per_section;
	extern size_t ft_section_depth;

	/*
		Constants only configurable at compilation time.
	*/

	// Full text indexer config
	inline const unsigned long long ft_num_shards = 1024;
	inline const unsigned long long ft_num_partitions = 1;
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

	// Other constants.
	inline const unsigned long long num_async_file_transfers = 48;
	inline const std::string test_data_path = "/var/www/html/node0003.alexandria.org/test-data/";

	// Commoncrawl parser.
	inline const std::string cc_target_output = "alexandria-cc-output";
	inline const bool cc_run_on_lambda = true;

	inline const std::string log_file_path = "/var/log/alexandria.log";

	void read_config(const std::string &config_file);

}


