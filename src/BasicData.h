
#pragma once

#include <iostream>
#include <istream>
#include <fstream>
#include <vector>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include "SubSystem.h"

using namespace boost::iostreams;
using namespace std;

class BasicData {

public:

	BasicData();
	BasicData(const SubSystem *sub_system);

	void read_stream(basic_iostream< char, std::char_traits< char > > &stream);
	void read_stream(ifstream &stream);
	void download(const string &bucket, const string &key);

protected:

	vector<string> m_data;
	const SubSystem *m_sub_system;

	void read_data(filtering_istream &decompress_stream);

};
