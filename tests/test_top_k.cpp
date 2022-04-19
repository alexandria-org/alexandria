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
#include <algorithm/top_k.h>

BOOST_AUTO_TEST_SUITE(test_top_k)

BOOST_AUTO_TEST_CASE(test_1) {
	const std::vector<int> res = ::algorithm::top_k<int>({1,2,3,4,5,6}, 2);
	bool is_correct = (res == std::vector<int>{5,6} || res == std::vector<int>{6,5});
	BOOST_CHECK(is_correct);
}

BOOST_AUTO_TEST_CASE(test_2) {
	const std::vector<int> res = ::algorithm::top_k<int>({1,2,3,4,5,6,7}, 2);
	bool is_correct = (res == std::vector<int>{6,7} || res == std::vector<int>{7,6});
	BOOST_CHECK(is_correct);
}

BOOST_AUTO_TEST_CASE(test_3) {
	const std::vector<int> res = ::algorithm::top_k<int>({}, 2);
	bool is_correct = (res == std::vector<int>{});
	BOOST_CHECK(is_correct);
}

BOOST_AUTO_TEST_CASE(test_4) {
	const std::vector<int> res = ::algorithm::top_k<int>({2,3,1}, 2);
	bool is_correct = (res == std::vector<int>{2,3} || res == std::vector<int>{3,2});
	BOOST_CHECK(is_correct);
}

BOOST_AUTO_TEST_CASE(test_5) {
	const std::vector<int> res = ::algorithm::top_k<int>({7,5,3,4,4,8,4,1,1,3,4}, 3);

	bool is_correct = true;
	for (int i : res) {
		if (i < 5) is_correct = false;
	}
	BOOST_CHECK(is_correct);
}

BOOST_AUTO_TEST_CASE(test_6) {
	const std::vector<int> res = ::algorithm::top_k<int>({7,5,3,4,4,8,4,1,1,3,4}, 6);

	bool is_correct = true;
	for (int i : res) {
		if (i < 4) is_correct = false;
	}
	BOOST_CHECK(is_correct);
}

BOOST_AUTO_TEST_CASE(test_7) {
	const std::vector<int> res = ::algorithm::top_k<int>({1,3,0,1,4,3,9,2,0,3}, 1);

	bool is_correct = res == std::vector<int>{9};
	BOOST_CHECK(is_correct);
}

BOOST_AUTO_TEST_CASE(test_8) {
	const std::vector<int> res = ::algorithm::top_k<int>({1,3,0,1,4,3,9,2,0,3}, 3, [](const int &a, const int &b) {
		return a > b;
	});

	bool is_correct = true;
	for (int i : res) {
		if (i > 1) is_correct = false;
	}
	BOOST_CHECK(is_correct);
}

BOOST_AUTO_TEST_SUITE_END()
