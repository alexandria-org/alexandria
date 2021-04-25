
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

class CCIndex : public TextBase {

public:
	CCIndex();
	~CCIndex();

	void read_stream(basic_iostream< char, std::char_traits< char > > &stream);
	void read_stream(ifstream &stream);

private:

	vector<vector<string>> m_data;

	void read_data(filtering_istream &decompress_stream);

};
