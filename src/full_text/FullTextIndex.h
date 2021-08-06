
#pragma once

#define FT_NUM_SHARDS 1024
#define FULL_TEXT_MAX_KEYS 0xFFFFFFFF
#define FT_INDEXER_MAX_CACHE_GB 30
#define FT_NUM_THREADS_INDEXING 48
#define FT_NUM_THREADS_MERGING 24
#define FT_INDEXER_CACHE_BYTES_PER_SHARD ((FT_INDEXER_MAX_CACHE_GB * 1000ul*1000ul*1000ul) / (FT_NUM_SHARDS * FT_NUM_THREADS_INDEXING))

template<typename DataRecord> class FullTextIndex;

#include <iostream>
#include <vector>

#include "parser/URL.h"
#include "system/SubSystem.h"
#include "system/ThreadPool.h"
#include "Scores.h"
#include "FullTextRecord.h"
#include "FullTextShard.h"
#include "FullTextResultSet.h"
#include "SearchMetric.h"
#include "link_index/LinkFullTextRecord.h"
#include "text/Text.h"

#include "system/Logger.h"

using namespace std;

template<typename DataRecord>
class FullTextIndex {

public:
	FullTextIndex(const string &name);
	~FullTextIndex();

	void find_score(const string &word, URL &url);
	float find_lowest_score(const string &word);
	void read_num_results(const string &word, size_t limit);

	size_t disk_size() const;

	const vector<FullTextShard<DataRecord> *> &shards() { return m_shards; };

private:

	string m_db_name;
	vector<FullTextShard<DataRecord> *> m_shards;

};

template<typename DataRecord>
FullTextIndex<DataRecord>::FullTextIndex(const string &db_name)
: m_db_name(db_name)
{
	for (size_t shard_id = 0; shard_id < FT_NUM_SHARDS; shard_id++) {
		m_shards.push_back(new FullTextShard<DataRecord>(m_db_name, shard_id));
	}
}

template<typename DataRecord>
FullTextIndex<DataRecord>::~FullTextIndex() {
	for (FullTextShard<DataRecord> *shard : m_shards) {
		delete shard;
	}
}

template<typename DataRecord>
size_t FullTextIndex<DataRecord>::disk_size() const {
	size_t size = 0;
	for (auto shard : m_shards) {
		size += shard->disk_size();
	}
	return size;
}

