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

#include "algorithm/Algorithm.h"

BOOST_AUTO_TEST_SUITE(algorithm)

BOOST_AUTO_TEST_CASE(intersection) {

	{
		const vector<int> result = Algorithm::intersection({
			{1, 2, 3},
			{2, 3},
			{2, 3, 4}
		});

		BOOST_CHECK_EQUAL(2, result.size());
		BOOST_CHECK_EQUAL(2, result[0]);
		BOOST_CHECK_EQUAL(3, result[1]);
	}

	{
		const vector<int> result = Algorithm::intersection({
			{1, 2, 3, 5},
			{2, 3, 5, 7},
			{2, 3, 4, 5}
		});

		BOOST_CHECK_EQUAL(3, result.size());
		BOOST_CHECK_EQUAL(2, result[0]);
		BOOST_CHECK_EQUAL(3, result[1]);
		BOOST_CHECK_EQUAL(5, result[2]);
	}

	{
		const vector<int> result = Algorithm::intersection({});

		BOOST_CHECK_EQUAL(0, result.size());
	}

	{
		const vector<int> result = Algorithm::intersection({
			{1, 2, 3, 5, 6, 7, 8},
			{9, 10},
			{1, 2, 3, 4, 5}
		});

		BOOST_CHECK_EQUAL(0, result.size());
	}
}

BOOST_AUTO_TEST_SUITE_END();
