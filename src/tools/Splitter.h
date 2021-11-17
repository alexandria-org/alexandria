
#pragma once

#include "config.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <thread>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/filesystem.hpp>
#include "full_text/FullText.h"
#include "algorithm/Algorithm.h"
#include "parser/URL.h"
#include "system/System.h"

namespace Tools {

	void run_splitter();

}

