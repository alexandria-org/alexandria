
#pragma once

#include <iostream>
#include <istream>
#include <vector>
#include <mutex>
#include "common/common.h"

#include "parser/URL.h"
#include "system/SubSystem.h"
#include "hash_table/HashTableShardBuilder.h"
#include "full_text/UrlToDomain.h"
#include "full_text/FullTextShardBuilder.h"
#include "DomainLinkFullTextRecord.h"

using namespace std;

class DomainLinkIndexer {

public:

	DomainLinkIndexer(int id, const string &db_name, const SubSystem *sub_system, UrlToDomain *url_to_domain);
	~DomainLinkIndexer();

	void add_stream(vector<HashTableShardBuilder *> &shard_builders, basic_istream<char> &stream, size_t partition);
	void write_cache(mutex *write_mutexes);
	void flush_cache(mutex *write_mutexes);

private:

	const string m_db_name;
	const SubSystem *m_sub_system;
	UrlToDomain *m_url_to_domain;
	int m_indexer_id;
	hash<string> m_hasher;

	vector<FullTextShardBuilder<DomainLinkFullTextRecord> *> m_shards;

	void add_data_to_shards(uint64_t link_hash, const URL &source_url, const URL &target_url, const string &link_text,
		float score);
	void add_expanded_data_to_shards(uint64_t link_hash, const URL &source_url, const URL &target_url, const string &link_text,
		float score);

};
