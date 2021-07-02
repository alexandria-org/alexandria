
#pragma once

#include <iostream>
#include <istream>
#include <vector>
#include <mutex>
#include "common/common.h"

#include "LinkShard.h"
#include "LinkShardBuilder.h"
#include "LinkIndex.h"
#include "parser/URL.h"
#include "abstract/TextBase.h"
#include "system/SubSystem.h"
#include "hash_table/HashTableShardBuilder.h"
#include "full_text/FullTextIndexer.h"

using namespace std;

class LinkIndexer : public TextBase {

public:

	LinkIndexer(int id, const SubSystem *sub_system, FullTextIndexer *ft_indexer);
	~LinkIndexer();

	void add_stream(vector<HashTableShardBuilder *> &shard_builders, basic_istream<char> &stream);
	void write_cache(mutex *write_mutexes);
	void flush_cache(mutex *write_mutexes);

private:

	const SubSystem *m_sub_system;
	FullTextIndexer *m_ft_indexer;
	int m_indexer_id;
	hash<string> m_hasher;
	vector<LinkShardBuilder *> m_shards;

	void add_data_to_shards(uint64_t link_hash, const URL &source_url, const URL &target_url, const string &link_text,
		uint32_t score);
	void adjust_score_for_domain_link(const string &link_text, const URL &source_url, const URL &target_url,
		int source_harmonic);

};
