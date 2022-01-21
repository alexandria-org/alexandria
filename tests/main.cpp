/*
 * MIT License
 *
 * Alexandria.org
 *
 * Copyright (c) 2021 Josef Cullhed, <info@alexandria.org>, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define BOOST_TEST_MODULE "Unit tests for alexandria.org"

#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>

#include "config.h"

#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <streambuf>
#include <math.h>
#include <vector>
#include <set>
#include <map>

using std::string;
using std::vector;
using std::ifstream;
using std::stringstream;
using std::set;
using std::map;
using std::pair;

#include "search_allocation.h"
#include "file.h"
#include "url.h"
#include "html_parser.h"
#include "unicode.h"
#include "text.h"
#include "sub_system.h"
#include "hash_table.h"
#include "full_text.h"
#include "api.h"
#include "search_engine.h"
#include "configuration.h"
#include "performance.h"
#include "sort.h"
#include "algorithm.h"
#include "deduplication.h"
#include "sections.h"
#include "logger.h"
#include "hyper_log_log.h"
#include "hyper_ball.h"
#include "cluster.h"
#include "cc_parser.h"
#include "hash.h"
#include "shard_builder.h"
#include "n_gram.h"
#include "link_counter.h"
#include "url_store.h"
#include "key_value_store.h"

void run_before() {
	Logger::start_logger_thread();
	Worker::start_urlstore_server();
}

void run_after() {
	Logger::join_logger_thread();
}


int BOOST_TEST_CALL_DECL
main(int argc, char* argv[]) {

	run_before();

    int ret = ::boost::unit_test::unit_test_main( &init_unit_test, argc, argv );

	run_after();

	return ret;
}

