
#pragma once

#include <iostream>
#include <istream>
#include <vector>
#include <mutex>
#include "common/common.h"

#include "LinkIndex.h"
#include "parser/URL.h"
#include "system/SubSystem.h"
#include "hash_table/HashTableShardBuilder.h"
#include "full_text/FullTextIndexer.h"
#include "full_text/FullTextShardBuilder.h"

using namespace std;

class LinkIndexer {

public:

	LinkIndexer(int id, const string &db_name, const SubSystem *sub_system, FullTextIndexer *ft_indexer);
	~LinkIndexer();

	void add_stream(vector<HashTableShardBuilder *> &shard_builders, basic_istream<char> &stream);
	void write_cache(mutex *write_mutexes);
	void flush_cache(mutex *write_mutexes);

private:

	const string m_db_name;
	const SubSystem *m_sub_system;
	FullTextIndexer *m_ft_indexer;
	int m_indexer_id;
	hash<string> m_hasher;

	vector<FullTextShardBuilder<LinkFullTextRecord> *> m_shards;
	vector<FullTextShardBuilder<FullTextRecord> *> m_adjustment_shards;
	vector<FullTextShardBuilder<FullTextRecord> *> m_domain_adjustment_shards;

	void add_data_to_shards(uint64_t link_hash, const URL &source_url, const URL &target_url, const string &link_text,
		float score);
	void add_expanded_data_to_shards(uint64_t link_hash, const URL &source_url, const URL &target_url, const string &link_text,
		float score);
	void add_domain_link(const string &link_text, const Link &link);
	void add_url_link(const string &link_text, const Link &link);

};
