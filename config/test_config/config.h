
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
	const unsigned long long ft_num_partitions = 8;
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

	// Other constants.
	const unsigned long long num_async_file_transfers = 48;

}

