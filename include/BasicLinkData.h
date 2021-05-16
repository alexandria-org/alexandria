
#pragma once

#include <iostream>
#include <istream>
#include <vector>
#include <map>

#include <fstream>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include "TextBase.h"
#include "BasicData.h"
#include "URL.h"

using namespace boost::iostreams;
using namespace std;

class BasicLinkData : public BasicData, public TextBase {

public:
	BasicLinkData();
	BasicLinkData(const SubSystem *);
	~BasicLinkData();

	void build_index(const string &output_file_name);

	void add_to_index(const string &word, const string from_domain, const string &from_uri,
		const string &to_domain, const string &to_uri, const string &link_text);
	inline bool is_in_dictionary(const string &word);

};
