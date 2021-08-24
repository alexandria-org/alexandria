
#pragma once

namespace Config {

	// Cluster config
	const unsigned long long nodes_in_cluster = 3;
	const unsigned long long node_id = 0;

	// Full text indexer config
	const unsigned long long ft_num_shards = 1024;
	const unsigned long long ft_max_keys = 0xFFFFFFFF;
	const unsigned long long ft_max_cache_gb = 30;
	const unsigned long long ft_num_threads_indexing = 48;
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

}

