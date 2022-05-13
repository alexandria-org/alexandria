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
	bf.commit();
	BOOST_CHECK(bf.exists("test"));
	BOOST_CHECK(!bf.exists("test2"));

	bf.insert("test2");
	bf.commit();
	BOOST_CHECK(bf.exists("test2"));
}

BOOST_AUTO_TEST_CASE(test_bloom_filter_merge) {

	algorithm::bloom_filter bf1;
	bf1.insert("test1");
	bf1.insert("test2");
	bf1.commit();

	algorithm::bloom_filter bf2;
	bf2.insert("test3");
	bf2.insert("test4");
	bf2.commit();

	bf1.merge(bf2);

	BOOST_CHECK(bf1.exists("test1"));
	BOOST_CHECK(bf1.exists("test2"));
	BOOST_CHECK(bf1.exists("test3"));
	BOOST_CHECK(bf1.exists("test4"));

	BOOST_CHECK(!bf1.exists("test0"));
	BOOST_CHECK(!bf1.exists("test5"));
	BOOST_CHECK(!bf1.exists("random"));
	BOOST_CHECK(!bf1.exists("random2"));
}

BOOST_AUTO_TEST_CASE(test_bloom_filter_save) {
	{
		algorithm::bloom_filter bf;
		bf.insert("test1");
		bf.insert("test2");
		bf.write_file("/tmp/bloom");
	}

	{
		algorithm::bloom_filter bf;
		bf.read_file("/tmp/bloom");

		BOOST_CHECK(bf.exists("test1"));
		BOOST_CHECK(bf.exists("test2"));
		BOOST_CHECK(!bf.exists("test3"));
	}
}

BOOST_AUTO_TEST_SUITE_END()
