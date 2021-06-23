
#pragma once

#include <iostream>
#include <istream>
#include <vector>
#include <mutex>
#include "common/common.h"

#include "FullTextShard.h"
#include "FullTextShardBuilder.h"
#include "FullTextIndex.h"
#include "parser/URL.h"
#include "abstract/TextBase.h"
#include "system/SubSystem.h"
#include "hash_table/HashTableShardBuilder.h"

using namespace std;

class FullTextIndexer : public TextBase {

public:

	FullTextIndexer(int id, const SubSystem *sub_system);
	~FullTextIndexer();

	void add_stream(vector<HashTableShardBuilder *> &shard_builders, basic_istream<char> &stream,
		const vector<size_t> &cols, const vector<uint32_t> &scores);
	void add_text(vector<HashTableShardBuilder *> &shard_builders, const string &key, const string &text,
		uint32_t score);
	void write_cache(mutex *write_mutexes);
	void flush_cache(mutex *write_mutexes);

private:

	const SubSystem *m_sub_system;
	int m_indexer_id;
	hash<string> m_hasher;
	vector<FullTextShardBuilder *> m_shards;

	void add_data_to_shards(const string &key, const string &text, uint32_t score);

};
