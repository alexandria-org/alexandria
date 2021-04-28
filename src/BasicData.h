
#pragma once

#include <iostream>
#include <istream>
#include <fstream>
#include <vector>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

using namespace boost::iostreams;
using namespace std;

class BasicData {

public:

	void read_stream(basic_iostream< char, std::char_traits< char > > &stream);
	void read_stream(ifstream &stream);

protected:

	vector<string> m_data;

	void read_data(filtering_istream &decompress_stream);

};
