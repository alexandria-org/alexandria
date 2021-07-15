
#pragma once

#define LI_NUM_SHARDS 8192
#define LI_KEY_LEN 8
#define LI_SCORE_LEN 4
#define LI_RECORD_LEN (8+8+8+8+4)

#define LI_INDEXER_MAX_CACHE_GB 15
#define LI_NUM_THREADS_INDEXING 48
#define LI_NUM_THREADS_MERGING 24
#define LI_INDEXER_CACHE_BYTES_PER_SHARD ((LI_INDEXER_MAX_CACHE_GB * 1000ul*1000ul*1000ul) / (LI_NUM_SHARDS * LI_NUM_THREADS_INDEXING))
#define LI_INDEXER_MAX_CACHE_SIZE 500

#include <iostream>
#include <vector>

#include "system/SubSystem.h"
#include "abstract/TextBase.h"
#include "system/ThreadPool.h"
#include "LinkResult.h"
#include "LinkFullTextRecord.h"
#include "full_text/FullTextShard.h"
#include "full_text/FullTextResultSet.h"

using namespace std;

class LinkIndex : public TextBase {

public:
	LinkIndex(const string &name);
	~LinkIndex();

	vector<LinkResult> search_phrase(const string &phrase, int limit, size_t &total_found);

	size_t disk_size() const;

	void download(const SubSystem *sub_system);
	void upload(const SubSystem *sub_system);

	// Testable private functions.
	vector<size_t> value_intersection(const map<size_t, FullTextResultSet<LinkFullTextRecord> *> &values_map,
		size_t &shortest_vector_position) const;

private:

	string m_db_name;
	hash<string> m_hasher;

	vector<FullTextShard<LinkFullTextRecord> *> m_shards;

	void sort_results(vector<LinkResult> &results);
	void run_upload_thread(const SubSystem *sub_system, const FullTextShard<LinkFullTextRecord> *shard);
	void run_download_thread(const SubSystem *sub_system, const FullTextShard<LinkFullTextRecord> *shard);

};
