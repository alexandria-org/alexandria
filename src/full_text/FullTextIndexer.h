
#pragma once

#include <iostream>
#include <istream>
#include <vector>
#include <mutex>
#include <unordered_map>
#include "common/common.h"

class FullTextIndexer;

#include "FullTextShard.h"
#include "FullTextShardBuilder.h"
#include "FullTextIndex.h"
#include "AdjustmentList.h"
#include "UrlToDomain.h"
#include "parser/URL.h"
#include "abstract/TextBase.h"
#include "system/SubSystem.h"
#include "hash_table/HashTableShardBuilder.h"
#include "link_index/Link.h"
#include "FullTextRecord.h"

using namespace std;

class FullTextIndexer : public TextBase {

public:

	FullTextIndexer(int id, const string &db_name, const SubSystem *sub_system, UrlToDomain *url_to_domain);
	~FullTextIndexer();

	void add_stream(vector<HashTableShardBuilder *> &shard_builders, basic_istream<char> &stream,
		const vector<size_t> &cols, const vector<float> &scores);
	void add_link_stream(vector<HashTableShardBuilder *> &shard_builders, basic_istream<char> &stream);
	void add_text(vector<HashTableShardBuilder *> &shard_builders, const string &key, const string &text,
		float score);
	size_t write_cache(mutex *write_mutexes);
	size_t write_large(mutex *write_mutexes);
	void flush_cache(mutex *write_mutexes);
	void read_url_to_domain();
	void write_url_to_domain();
	void add_domain_link(uint64_t word_hash, const Link &link);
	void add_url_link(uint64_t word_hash, const Link &link);

	void write_adjustments_cache(mutex *write_mutexes);
	void flush_adjustments_cache(mutex *write_mutexes);

	bool has_key(uint64_t key) const {
		return m_url_to_domain->url_to_domain().count(key) > 0;
	}

	bool has_domain(uint64_t domain_hash) const {
		auto iter = m_url_to_domain->domains().find(domain_hash);
		if (iter == m_url_to_domain->domains().end()) return false;
		return iter->second > 0;
	}

	const UrlToDomain *url_to_domain() const {
		return m_url_to_domain;
	}

private:

	const SubSystem *m_sub_system;
	int m_indexer_id;
	const string m_db_name;
	hash<string> m_hasher;
	vector<FullTextShardBuilder<struct FullTextRecord> *> m_shards;

	UrlToDomain *m_url_to_domain = NULL;

	vector<AdjustmentList *> m_adjustments;
	const size_t m_adjustment_cache_limit = 50;

	void add_data_to_word_map(map<uint64_t, float> &word_map, const string &text, float score) const;
	void add_data_to_shards(const uint64_t &key_hash, const string &text, float score);

};
