
#pragma once

#include <iostream>
#include <istream>
#include <vector>

#include <fstream>
#include <iostream>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include "TextBase.h"

using namespace boost::iostreams;
using namespace std;

#define CC_COLUMN_URL 1
#define CC_COLUMN_TITLE 101
#define CC_COLUMN_H1 102
#define CC_COLUMN_META 103
#define CC_COLUMN_TEXT 104

class CCIndex : public TextBase {

public:
	CCIndex();
	~CCIndex();

	void read_stream(basic_iostream< char, std::char_traits< char > > &stream);
	void read_stream(ifstream &stream);
	void build_index();

private:

	vector<vector<string>> m_data;
	vector<int> m_columns;
	map<string, int> m_index;
	int m_group_by;

	void read_data(filtering_istream &decompress_stream);
	void index_word(const string &word, const string &group_by, int col_type);
	bool compare_numeric(pair<string, int>& a, pair<string, int>& b);

};
