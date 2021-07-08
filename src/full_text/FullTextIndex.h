
#pragma once

#define FT_NUM_SHARDS 8192
#define FULL_TEXT_MAX_KEYS 0xFFFFFFFF
#define FULL_TEXT_KEY_LEN 8
#define FULL_TEXT_SCORE_LEN 4
#define FULL_TEXT_RECORD_LEN 12
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

using namespace std;

class FullTextShard;

class FullTextIndex : public TextBase {

public:
	FullTextIndex(const string &name);
	~FullTextIndex();

	vector<FullTextResult> search_word(const string &word);
	vector<FullTextResult> search_phrase(const string &phrase, int limit, size_t &total_found);

	size_t disk_size() const;
	bool has_key(uint64_t key) const {
		return m_url_to_domain.count(key) > 0;
	}

	bool has_domain(uint64_t domain_hash) const {
		auto iter = m_domains.find(domain_hash);
		if (iter == m_domains.end()) return false;
		return iter->second > 0;
	}

	void download(const SubSystem *sub_system);
	void upload(const SubSystem *sub_system);

	// Testable private functions.
	vector<size_t> value_intersection(const map<size_t, FullTextResultSet *> &values_map,
		size_t &shortest_vector_position) const;
	
private:

	string m_db_name;
	hash<string> m_hasher;

	unordered_map<uint64_t, uint64_t> m_url_to_domain;
	unordered_map<uint64_t, size_t> m_domains;

	vector<FullTextShard *> m_shards;

	void sort_results(vector<FullTextResult> &results);
	void run_upload_thread(const SubSystem *sub_system, const FullTextShard *shard);
	void run_download_thread(const SubSystem *sub_system, const FullTextShard *shard);
	void read_url_to_domain();

};
