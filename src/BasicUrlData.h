
#pragma once

#include <iostream>
#include <istream>
#include <vector>
#include <map>

#include <fstream>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/algorithm/string.hpp>

#include "TextBase.h"
#include "BasicData.h"
#include "URL.h"
#include "TsvFile.h"

using namespace boost::iostreams;
using namespace std;

#define CC_HARMONIC_LIMIT 0

class BasicUrlData : public BasicData, public TextBase {

public:
	BasicUrlData();
	~BasicUrlData();

	void build_index(int id);
	inline string make_snippet(const string &text_after_h1);
	void load_domain_meta();
	void add_to_index(const string &word, const URL &url, const string &title, const string &snippet);
	inline bool is_in_dictionary(const string &word);
	void load_dictionary();

private:

	TsvFile m_domain_file;
	map<string, int> m_domain_meta;
	set<string> m_dictionary;
	stringstream m_result;


};
