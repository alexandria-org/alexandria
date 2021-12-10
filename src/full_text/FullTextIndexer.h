/*
 * MIT License
 *
 * Alexandria.org
 *
 * Copyright (c) 2021 Josef Cullhed, <info@alexandria.org>, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <iostream>
#include <istream>
#include <vector>
#include <mutex>
#include <unordered_map>

class FullTextIndexer;

#include "FullTextShard.h"
#include "FullTextShardBuilder.h"
#include "FullTextIndex.h"
#include "UrlToDomain.h"
#include "parser/URL.h"
#include "system/SubSystem.h"
#include "hash_table/HashTableShardBuilder.h"
#include "link_index/Link.h"
#include "FullTextRecord.h"

using namespace std;

class FullTextIndexer {

public:

	FullTextIndexer(int id, const string &db_name, size_t partition, const SubSystem *sub_system, UrlToDomain *url_to_domain);
	~FullTextIndexer();

	size_t add_stream(vector<HashTableShardBuilder *> &shard_builders, basic_istream<char> &stream,
		const vector<size_t> &cols, const vector<float> &scores, size_t partition, const string &batch);
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

	int m_indexer_id;
	const string m_db_name;
	const SubSystem *m_sub_system;
	hash<string> m_hasher;
	vector<FullTextShardBuilder<struct FullTextRecord> *> m_shards;

	UrlToDomain *m_url_to_domain = NULL;

	void add_expanded_data_to_word_map(map<uint64_t, float> &word_map, const string &text, float score) const;
	void add_data_to_word_map(map<uint64_t, float> &word_map, const string &text, float score) const;
	void add_data_to_shards(const URL &url, const string &text, float score);

};
