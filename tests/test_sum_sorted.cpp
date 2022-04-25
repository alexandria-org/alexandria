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
#include <vector>
#include "algorithm/sum_sorted.h"
#include "indexer/counted_record.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(test_sum_sorted, * boost::unit_test::tolerance(0.00001))

BOOST_AUTO_TEST_CASE(test_sum_sorted1) {

	vector<vector<int>> sorted = {
		{1, 2, 3},
		{2, 3},
		{3}
	};
	vector<int> res = ::algorithm::sum_sorted<int>(sorted, [](int &a, const int &b) {
		a += b;
	});

	BOOST_REQUIRE(res.size() == 3);
	BOOST_CHECK(res[0] == 1);
	BOOST_CHECK(res[1] == 4);
	BOOST_CHECK(res[2] == 9);
}

BOOST_AUTO_TEST_CASE(test_sum_sorted2) {

	vector<vector<int>> sorted = {
		{3},
		{2, 3},
		{1, 2, 3},
	};
	vector<int> res = ::algorithm::sum_sorted<int>(sorted, [](int &a, const int &b) {
		a += b;
	});

	BOOST_REQUIRE(res.size() == 3);
	BOOST_CHECK(res[0] == 1);
	BOOST_CHECK(res[1] == 4);
	BOOST_CHECK(res[2] == 9);
}

BOOST_AUTO_TEST_CASE(test_sum_sorted3) {

	vector<vector<indexer::counted_record>> sorted = {
		{indexer::counted_record(3, 0.1)},
		{indexer::counted_record(2, 0.1), indexer::counted_record(3, 0.1)},
		{indexer::counted_record(1, 0.1), indexer::counted_record(2, 0.1), indexer::counted_record(3, 0.1)},
	};
	vector<indexer::counted_record> res = ::algorithm::sum_sorted<indexer::counted_record>(sorted,
			[](indexer::counted_record &a, const indexer::counted_record &b) {
		a.m_score += b.m_score;
	});

	BOOST_REQUIRE(res.size() == 3);
	BOOST_CHECK_EQUAL(res[0].m_score, 0.1f);
	BOOST_CHECK_EQUAL(res[1].m_score, 0.2f);
	BOOST_CHECK_EQUAL(res[2].m_score, 0.3f);
}

BOOST_AUTO_TEST_CASE(test_sum_sorted4) {

	vector<vector<indexer::counted_record>> sorted = {
		{indexer::counted_record(1, 0.1), indexer::counted_record(2, 0.2), indexer::counted_record(3, 0.3)},
		{indexer::counted_record(10, 0.4), indexer::counted_record(25, 0.5), indexer::counted_record(30, 0.6)},
		{indexer::counted_record(1, 0.7), indexer::counted_record(25, 0.8), indexer::counted_record(40, 0.9)},
	};
	vector<indexer::counted_record> res = ::algorithm::sum_sorted<indexer::counted_record>(sorted,
			[](indexer::counted_record &a, const indexer::counted_record &b) {
		a.m_score += b.m_score;
	});

	BOOST_REQUIRE(res.size() == 7);
	BOOST_CHECK_EQUAL(res[0].m_score, 0.8f);
	BOOST_CHECK_EQUAL(res[1].m_score, 0.2f);
	BOOST_CHECK_EQUAL(res[2].m_score, 0.3f);
	BOOST_CHECK_EQUAL(res[3].m_score, 0.4f);
	BOOST_CHECK_EQUAL(res[4].m_score, 1.3f);
	BOOST_CHECK_EQUAL(res[5].m_score, 0.6f);
	BOOST_CHECK_EQUAL(res[6].m_score, 0.9f);

	BOOST_CHECK_EQUAL(res[0].m_value, 1);
	BOOST_CHECK_EQUAL(res[1].m_value, 2);
	BOOST_CHECK_EQUAL(res[2].m_value, 3);
	BOOST_CHECK_EQUAL(res[3].m_value, 10);
	BOOST_CHECK_EQUAL(res[4].m_value, 25);
	BOOST_CHECK_EQUAL(res[5].m_value, 30);
	BOOST_CHECK_EQUAL(res[6].m_value, 40);
}

BOOST_AUTO_TEST_SUITE_END()
