
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

#define CC_COLUMN_URL 1
#define CC_COLUMN_TITLE 101
#define CC_COLUMN_H1 102
#define CC_COLUMN_META 103
#define CC_COLUMN_TEXT 104

class BasicUrlData : public BasicData, public TextBase {

public:
	BasicUrlData();
	~BasicUrlData();

	void build_index();

private:


};
