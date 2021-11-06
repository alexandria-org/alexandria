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

BOOST_AUTO_TEST_CASE(incremental_partitions) {

	{
		vector<vector<int>> res = Algorithm::incremental_partitions({5}, 64);
		BOOST_CHECK_EQUAL(res.size(), 5);
	}
	{
		vector<vector<int>> res = Algorithm::incremental_partitions({6}, 64);
		BOOST_CHECK_EQUAL(res.size(), 6);
	}
	{
		vector<vector<int>> res = Algorithm::incremental_partitions({3}, 64);
		BOOST_CHECK_EQUAL(res.size(), 3);
		BOOST_CHECK(res[0] == vector<int>{0});
		BOOST_CHECK(res[1] == vector<int>{1});
		BOOST_CHECK(res[2] == vector<int>{2});
	}

	{
		vector<vector<int>> res = Algorithm::incremental_partitions({2, 2}, 64);
		BOOST_CHECK_EQUAL(res.size(), 4);
		BOOST_CHECK((res[0] == vector<int>{0, 0}));
		BOOST_CHECK((res[1] == vector<int>{1, 0}));
		BOOST_CHECK((res[2] == vector<int>{0, 1}));
		BOOST_CHECK((res[3] == vector<int>{1, 1}));
	}
	{
		vector<vector<int>> res = Algorithm::incremental_partitions({3, 3}, 64);
		BOOST_CHECK_EQUAL(res.size(), 9);
		BOOST_CHECK((res[0] == vector<int>{0, 0}));
		BOOST_CHECK((res[1] == vector<int>{1, 0}));
		BOOST_CHECK((res[2] == vector<int>{0, 1}));
		BOOST_CHECK((res[3] == vector<int>{1, 1}));
		BOOST_CHECK((res[4] == vector<int>{2, 0}));
		BOOST_CHECK((res[5] == vector<int>{0, 2}));
		BOOST_CHECK((res[6] == vector<int>{2, 1}));
		BOOST_CHECK((res[7] == vector<int>{1, 2}));
		BOOST_CHECK((res[8] == vector<int>{2, 2}));
	}
	{
		vector<vector<int>> res = Algorithm::incremental_partitions({3, 3}, 5);
		BOOST_CHECK_EQUAL(res.size(), 5);
		BOOST_CHECK((res[0] == vector<int>{0, 0}));
		BOOST_CHECK((res[1] == vector<int>{1, 0}));
		BOOST_CHECK((res[2] == vector<int>{0, 1}));
		BOOST_CHECK((res[3] == vector<int>{1, 1}));
		BOOST_CHECK((res[4] == vector<int>{2, 0}));
	}
	{
		vector<vector<int>> res = Algorithm::incremental_partitions({3, 3, 3}, 64);
		BOOST_CHECK_EQUAL(res.size(), 27);
		BOOST_CHECK((res[0] == vector<int>{0, 0, 0}));
		BOOST_CHECK((res[1] == vector<int>{1, 0, 0}));
		BOOST_CHECK((res[2] == vector<int>{0, 1, 0}));
		BOOST_CHECK((res[3] == vector<int>{0, 0, 1}));
		BOOST_CHECK((res[4] == vector<int>{1, 1, 0}));
		BOOST_CHECK((res[5] == vector<int>{1, 0, 1}));
		BOOST_CHECK((res[6] == vector<int>{0, 1, 1}));
		BOOST_CHECK((res[7] == vector<int>{2, 0, 0}));
		BOOST_CHECK((res[8] == vector<int>{0, 2, 0}));
		BOOST_CHECK((res[9] == vector<int>{0, 0, 2}));
		BOOST_CHECK((res[10] == vector<int>{1, 1, 1}));
		BOOST_CHECK((res[11] == vector<int>{2, 1, 0}));
		BOOST_CHECK((res[12] == vector<int>{2, 0, 1}));
		BOOST_CHECK((res[13] == vector<int>{1, 2, 0}));
		BOOST_CHECK((res[14] == vector<int>{1, 0, 2}));
		BOOST_CHECK((res[15] == vector<int>{0, 2, 1}));
	}
	{
		vector<vector<int>> res = Algorithm::incremental_partitions({2, 3}, 64);
		BOOST_CHECK_EQUAL(res.size(), 6);
		BOOST_CHECK((res[0] == vector<int>{0, 0}));
		BOOST_CHECK((res[1] == vector<int>{1, 0}));
		BOOST_CHECK((res[2] == vector<int>{0, 1}));
		BOOST_CHECK((res[3] == vector<int>{1, 1}));
		BOOST_CHECK((res[4] == vector<int>{0, 2}));
		BOOST_CHECK((res[5] == vector<int>{1, 2}));
	}

}

BOOST_AUTO_TEST_SUITE_END();
