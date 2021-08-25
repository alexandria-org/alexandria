
#pragma once

#include <vector>
#include <string>

namespace Config {

	// Cluster config
	inline const unsigned long long nodes_in_cluster = 3;
	inline const unsigned long long node_id = 0;

	inline const std::vector<std::string> batches = {
		"ALEXANDRIA-MANUAL-01",
		"CC-MAIN-2021-31"
	};

	inline const std::vector<std::string> link_batches = {
		"CC-MAIN-2021-31"
	};

	// Full text indexer config
	inline const unsigned long long ft_num_shards = 1024;
	inline const unsigned long long ft_num_partitions = 8;
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
	inline const unsigned long long ht_num_shards = 16384;
	inline const unsigned long long ht_num_buckets = 8;
	inline const unsigned long long ht_key_size = 8;

	// Other constants.
	inline const unsigned long long num_async_file_transfers = 48;

}

