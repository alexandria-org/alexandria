
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
#include "SubSystem.h"

using namespace boost::iostreams;
using namespace std;

#define CC_HARMONIC_LIMIT 0

class BasicUrlData : public BasicData, public TextBase {

public:
	BasicUrlData() {};
	BasicUrlData(const SubSystem *);
	~BasicUrlData();

	string build_index(int shard, int id);
	inline string make_snippet(const string &text_after_h1);
	void add_to_index(const string &word, const URL &url, const string &title, const string &snippet);
	inline bool is_in_dictionary(const string &word);

private:

	stringstream m_result;

	string get_output_filename(int shard, int id);

};
