
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

using namespace boost::iostreams;
using namespace std;

class BasicLinkData : public BasicData, public TextBase {

public:
	BasicLinkData();
	~BasicLinkData();

	void build_index();

private:

};
