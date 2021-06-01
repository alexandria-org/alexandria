
#pragma once

#include <iostream>
#include <istream>
#include <vector>
#include "common/common.h"

#include "FullTextShard.h"
#include "FullTextIndex.h"
#include "abstract/TextBase.h"

using namespace std;

#define FT_INDEXER_MAX_CACHE_SIZE 1000000

class FullTextIndexer : public TextBase {

public:

	FullTextIndexer();
	~FullTextIndexer();

	void add_stream(basic_istream<char> &stream, const vector<size_t> &cols, const vector<uint32_t> &scores);
	bool should_write_cache() const;
	void write_cache();

private:

	hash<string> m_hasher;
	vector<FullTextShard *> m_shards;

	void add_data_to_shards(const string &key, const string &text, uint32_t score);

};