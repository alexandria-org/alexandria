
#pragma once

#include <iostream>
#include <istream>
#include <vector>
#include <mutex>
#include <unordered_map>
#include "common/common.h"

#include "FullTextShard.h"
#include "FullTextShardBuilder.h"
#include "FullTextIndex.h"
#include "AdjustmentList.h"
#include "parser/URL.h"
#include "abstract/TextBase.h"
#include "system/SubSystem.h"
#include "hash_table/HashTableShardBuilder.h"
#include "link_index/Link.h"

using namespace std;

class FullTextShardBuilder;

class FullTextIndexer : public TextBase {

public:

	FullTextIndexer(int id, const string &db_name, const SubSystem *sub_system);
	~FullTextIndexer();

	void add_stream(vector<HashTableShardBuilder *> &shard_builders, basic_istream<char> &stream,
		const vector<size_t> &cols, const vector<uint32_t> &scores);
	void add_link_stream(vector<HashTableShardBuilder *> &shard_builders, basic_istream<char> &stream);
	void add_text(vector<HashTableShardBuilder *> &shard_builders, const string &key, const string &text,
		uint32_t score);
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
		return m_url_to_domain.count(key) > 0;
	}

	bool has_domain(uint64_t domain_hash) const {
		auto iter = m_domains.find(domain_hash);
		if (iter == m_domains.end()) return false;
		return iter->second > 0;
	}

	const unordered_map<uint64_t, uint64_t> *url_to_domain() const {
		return &m_url_to_domain;
	}

private:

	const SubSystem *m_sub_system;
	int m_indexer_id;
	const string m_db_name;
	hash<string> m_hasher;
	vector<FullTextShardBuilder *> m_shards;

	unordered_map<uint64_t, uint64_t> m_url_to_domain;
	unordered_map<uint64_t, size_t> m_domains;

	vector<AdjustmentList *> m_adjustments;
	const size_t m_adjustment_cache_limit = 100000;

	void add_data_to_shards(const uint64_t &key_hash, const string &text, uint32_t score);

};
