
#pragma once

#include <iostream>
#include <vector>

#include "TextBase.h"
#include "FullTextShard.h"
#include "FullTextResult.h"

using namespace std;

class FullTextIndex : public TextBase {

public:
	FullTextIndex(const string &name);
	~FullTextIndex();

	vector<FullTextResult> search(const string &word);
	void add(const string &key, const string &text);
	void add(const string &key, const string &text, uint32_t score);
	void consume_stream(basic_iostream<char> &stream);
	void save();
	size_t size() const;

	void truncate();

private:

	string m_db_name;
	hash<string> m_hasher;

	const size_t m_num_shards = 1;
	vector<FullTextShard *> m_shards;

};
