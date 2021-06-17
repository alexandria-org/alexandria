
#pragma once

#include <iostream>
#include <vector>

#include "system/SubSystem.h"
#include "abstract/TextBase.h"
#include "FullTextShard.h"
#include "FullTextResult.h"
#include "system/ThreadPool.h"

using namespace std;

#define FT_NUM_BUCKETS 8
#define FT_NUM_SHARDS 2048

class FullTextShard;

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
	vector<size_t> value_intersection(const map<size_t, FullTextResultSet *> &values_map,
		size_t &shortest_vector_position) const;

private:

	string m_db_name;
	hash<string> m_hasher;

	vector<FullTextShard *> m_shards;

	void sort_results(vector<FullTextResult> &results);
	void run_upload_thread(const SubSystem *sub_system, const FullTextShard *shard);
	void run_download_thread(const SubSystem *sub_system, const FullTextShard *shard);

};
