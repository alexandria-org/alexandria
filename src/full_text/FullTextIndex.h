
#pragma once

#include <iostream>
#include <vector>

#include "abstract/TextBase.h"
#include "FullTextShard.h"
#include "FullTextResult.h"

using namespace std;

class FullTextIndex : public TextBase {

public:
	FullTextIndex(const string &name);
	~FullTextIndex();

	vector<FullTextResult> search_word(const string &word);
	vector<FullTextResult> search_phrase(const string &phrase);

	// Add single key/value to index.
	void add(const string &key, const string &text);
	void add(const string &key, const string &text, uint32_t score);

	/*
		Add stream with tab separated data. Cols is a vector of column indices to index and scores are scores for the
		corresponding column.

		Example:
		index.add_stream(stream, {0, 2}, {1, 10});
		Adds column 0 and 2 to the index with scores 1 and 10.
	*/
	void add_stream(basic_istream<char> &stream, const vector<size_t> &cols, const vector<uint32_t> &scores);

	void save();
	void truncate();

	// Getters.
	size_t size() const;

private:

	string m_db_name;
	hash<string> m_hasher;

	const size_t m_num_shards = 1;
	vector<FullTextShard *> m_shards;

};
