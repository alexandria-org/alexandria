
#pragma once

#include <iostream>
#include <vector>

#include "TextBase.h"
#include "FullTextShard.h"
#include "FullTextResult.h"

using namespace std;

class FullTextIndex : public TextBase {

public:
	FullTextIndex();
	~FullTextIndex();

	vector<FullTextResult> search(const string &word);
	void add(const string &key, const string &text);
	void consume_stream(basic_iostream<char> &stream);
	void save();

private:

	hash<string> m_hasher;

	const size_t m_num_shards = 16;
	vector<FullTextShard *> m_shards;

};

