
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

#include "BasicData.h"
#include "parser/URL.h"
#include "file/TsvFile.h"
#include "system/SubSystem.h"

using namespace std;
using namespace boost::iostreams;

#define CC_HARMONIC_LIMIT 0

class BasicUrlData : public BasicData {

public:
	BasicUrlData() {};
	BasicUrlData(const SubSystem *);
	~BasicUrlData();

	void build_index(const string &output_file_name);

	string build_full_text_index();
	inline string make_snippet(const string &text_after_h1);
	void add_to_index(const string &word, const URL &url, const string &title, const string &snippet);
	inline void add_to_full_text_index(const string &word, uint64_t id, float score);
	inline bool is_in_dictionary(const string &word);

private:

	stringstream m_full_text_result;
	map<size_t, string> m_index;
	vector<string> m_keys;
	size_t m_next_key = 0;

	string sort_and_store();
	string store_full_text();

};
