
#pragma once

#include <istream>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

using namespace std;
using namespace boost::iostreams;

class CCIndex {

public:
	CCIndex();
	~CCIndex();

	void read_stream(basic_iostream< char, std::char_traits< char > > &stream);

private:
	
	filtering_istream m_stream;

};
