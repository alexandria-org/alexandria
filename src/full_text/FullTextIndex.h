
#pragma once

#define FT_NUM_SHARDS 8192
#define FULL_TEXT_MAX_KEYS 0xFFFFFFFF
#define FULL_TEXT_KEY_LEN 8
#define FULL_TEXT_SCORE_LEN 4
#define FT_INDEXER_MAX_CACHE_GB 30
#define FT_NUM_THREADS_INDEXING 48
#define FT_NUM_THREADS_MERGING 24
#define FT_INDEXER_CACHE_BYTES_PER_SHARD ((FT_INDEXER_MAX_CACHE_GB * 1000ul*1000ul*1000ul) / (FT_NUM_SHARDS * FT_NUM_THREADS_INDEXING))

#include <iostream>
#include <vector>

#include "abstract/TextBase.h"
#include "system/SubSystem.h"
#include "system/ThreadPool.h"
#include "FullTextShard.h"
#include "FullTextResult.h"
#include "FullTextResultSet.h"
#include "FullTextRecord.h"

using namespace std;

class FullTextIndex : public TextBase {

public:
	FullTextIndex(const string &name);
	~FullTextIndex();

	vector<FullTextResult> search_word(const string &word);
	vector<FullTextResult> search_phrase(const string &phrase, int limit, size_t &total_found);

	size_t disk_size() const;

	void download(const SubSystem *sub_system);
	void upload(const SubSystem *sub_system);

	// Testable private functions.
	vector<size_t> value_intersection(const map<size_t, FullTextResultSet<FullTextRecord> *> &values_map,
		size_t &shortest_vector_position, vector<float> &scores) const;
	
private:

	string m_db_name;
	hash<string> m_hasher;

	vector<FullTextShard<FullTextRecord> *> m_shards;

	void sort_results(vector<FullTextResult> &results);
	void run_upload_thread(const SubSystem *sub_system, const FullTextShard<FullTextRecord> *shard);
	void run_download_thread(const SubSystem *sub_system, const FullTextShard<FullTextRecord> *shard);

};
