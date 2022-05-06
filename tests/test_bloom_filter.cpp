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

#include <boost/test/unit_test.hpp>
#include <fstream>
#include "algorithm/bloom_filter.h"
#include "algorithm/hash.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(test_bloom_filter)

BOOST_AUTO_TEST_CASE(test_bloom_filter) {
	algorithm::bloom_filter bf;

	bf.insert("test");
	BOOST_CHECK(bf.exists("test"));
	BOOST_CHECK(!bf.exists("test2"));

	bf.insert("test2");
	BOOST_CHECK(bf.exists("test2"));
}

BOOST_AUTO_TEST_CASE(test_bloom_filter_stress) {
	/*algorithm::bloom_filter bf;
	// Insert 10 billion strings. Measure false positive rate.
	std::cout << "inserting... size is: " << bf.size() << std::endl;
	for (size_t i = 0; i < 10000000000; i++) {
		if (i % 100000000 == 0) std::cout << "." << std::flush;
		bf.insert(std::to_string(i));
	}

	// save to file.
	ofstream outfile("/tmp/bloom", ios::binary | ios::trunc);
	outfile.write(bf.data(), bf.size());*/

	/*algorithm::bloom_filter bf;

	ifstream infile("/tmp/bloom", ios::binary);
	char *buf = new char[bf.size()];
	infile.read(buf, bf.size());
	bf.read(buf);
	delete buf;

	size_t num_test = 1000000;
	size_t num_false_positive = 0;
	for (size_t i = 0; i < num_test; i++) {
		if (bf.exists("test_" + to_string(i) + "_test")) {
			num_false_positive++;
		}
	}

	std::cout << "num_false_positive: " << num_false_positive << std::endl;

	std::cout << "false positive rate: " << ((double)num_false_positive / num_test) << "%" << std::endl;*/
}

BOOST_AUTO_TEST_SUITE_END()
