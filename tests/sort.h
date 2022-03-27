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

#include "algorithm/sort.h"

BOOST_AUTO_TEST_SUITE(test_sort)

struct test_data_struct1 {
	int data1;
	int data2;
};

BOOST_AUTO_TEST_CASE(merge_arrays) {

	{
		vector<int> arr1 = {1, 2, 3};
		vector<int> arr2 = {4, 5, 6};
		vector<int> arr3;
		vector<int> arr4{1, 2, 3, 4, 5, 6};

		algorithm::sort::merge_arrays(arr1, arr2, arr3);

		BOOST_CHECK(arr3 == arr4);
	}

	{
		vector<int> arr1 = {1, 2, 3};
		vector<int> arr2 = {3, 4, 5, 6};
		vector<int> arr3;
		vector<int> arr4{1, 2, 3, 3, 4, 5, 6};

		algorithm::sort::merge_arrays(arr1, arr2, arr3);

		BOOST_CHECK(arr3 == arr4);
	}

	{
		vector<int> arr1 = {};
		vector<int> arr2 = {3, 4, 5, 6};
		vector<int> arr3;
		vector<int> arr4{3, 4, 5, 6};

		algorithm::sort::merge_arrays(arr1, arr2, arr3);

		BOOST_CHECK(arr3 == arr4);
	}

}

BOOST_AUTO_TEST_CASE(merge_arrays_of_struct) {

	{
		vector<struct test_data_struct1> arr1{test_data_struct1{.data1 = 1, .data2 = 2}};
		vector<struct test_data_struct1> arr2{test_data_struct1{.data1 = 2, .data2 = 3}};
		vector<struct test_data_struct1> arr3;
		vector<struct test_data_struct1> arr4{test_data_struct1{.data1 = 1, .data2 = 2}, test_data_struct1{.data1 = 2, .data2 = 3}};

		algorithm::sort::merge_arrays(arr1, arr2, [](const struct test_data_struct1 &a, const struct test_data_struct1 &b) {
			return a.data1 < b.data1;
		}, arr3);

		BOOST_CHECK(arr3[0].data1 == arr4[0].data1 && arr3[0].data2 == arr4[0].data2);
		BOOST_CHECK(arr3[1].data1 == arr4[1].data1 && arr3[1].data2 == arr4[1].data2);
	}

	{
		vector<struct test_data_struct1> arr1{test_data_struct1{.data1 = 1, .data2 = 2}, test_data_struct1{.data1 = 3, .data2 = 4}};
		vector<struct test_data_struct1> arr2{test_data_struct1{.data1 = 2, .data2 = 3}};
		vector<struct test_data_struct1> arr3;
		vector<struct test_data_struct1> arr4{test_data_struct1{.data1 = 1, .data2 = 2}, test_data_struct1{.data1 = 2, .data2 = 3},
			test_data_struct1{.data1 = 3, .data2 = 4}};

		algorithm::sort::merge_arrays(arr1, arr2, [](const struct test_data_struct1 &a, const struct test_data_struct1 &b) {
			return a.data1 < b.data1;
		}, arr3);

		BOOST_CHECK(arr3[0].data1 == arr4[0].data1 && arr3[0].data2 == arr4[0].data2);
		BOOST_CHECK(arr3[1].data1 == arr4[1].data1 && arr3[1].data2 == arr4[1].data2);
		BOOST_CHECK(arr3[2].data1 == arr4[2].data1 && arr3[2].data2 == arr4[2].data2);
	}

}

BOOST_AUTO_TEST_CASE(merge_many_arrays) {

	{
		vector<int> arr1 = {1, 2, 3};
		vector<int> arr2 = {4, 5, 6};
		vector<int> arr3 = {7, 8, 9};
		vector<int> res;
		vector<vector<int>> inp{arr1, arr2, arr3};
		vector<int> corr{1, 2, 3, 4, 5, 6, 7, 8, 9};

		algorithm::sort::merge_arrays(inp, res);

		BOOST_CHECK(res == corr);
	}

	{
		vector<int> arr1 = {1, 3, 6};
		vector<int> arr2 = {2, 4, 9};
		vector<int> arr3 = {1, 5, 7, 8};
		vector<int> res;
		vector<vector<int>> inp{arr1, arr2, arr3};
		vector<int> corr{1, 1, 2, 3, 4, 5, 6, 7, 8, 9};

		algorithm::sort::merge_arrays(inp, res);

		BOOST_CHECK(res == corr);
	}
}

BOOST_AUTO_TEST_CASE(merge_many_arrays_of_struct) {

	{
		vector<struct test_data_struct1> arr1{
			test_data_struct1{.data1 = 1, .data2 = 11},
			test_data_struct1{.data1 = 2, .data2 = 12},
			test_data_struct1{.data1 = 3, .data2 = 13}
		};
		vector<struct test_data_struct1> arr2 = {
			test_data_struct1{.data1 = 4, .data2 = 14},
			test_data_struct1{.data1 = 5, .data2 = 15},
			test_data_struct1{.data1 = 6, .data2 = 16}
		};
		vector<struct test_data_struct1> arr3 = {
			test_data_struct1{.data1 = 7, .data2 = 17},
			test_data_struct1{.data1 = 8, .data2 = 18},
			test_data_struct1{.data1 = 9, .data2 = 19}
		};
		vector<struct test_data_struct1> res;
		vector<vector<struct test_data_struct1>> inp{arr1, arr2, arr3};
		vector<struct test_data_struct1> corr{
			test_data_struct1{.data1 = 1, .data2 = 11},
			test_data_struct1{.data1 = 2, .data2 = 12},
			test_data_struct1{.data1 = 3, .data2 = 13},
			test_data_struct1{.data1 = 4, .data2 = 14},
			test_data_struct1{.data1 = 5, .data2 = 15},
			test_data_struct1{.data1 = 6, .data2 = 16},
			test_data_struct1{.data1 = 7, .data2 = 17},
			test_data_struct1{.data1 = 8, .data2 = 18},
			test_data_struct1{.data1 = 9, .data2 = 19}
		};

		algorithm::sort::merge_arrays(inp, [](const struct test_data_struct1 &a, const struct test_data_struct1 &b) {
			return a.data1 < b.data1;
		}, res);

		BOOST_CHECK(corr.size() == res.size());
		for (size_t i = 0; i < corr.size(); i++) {
			BOOST_CHECK(res[i].data1 == corr[i].data1 && res[i].data2 == corr[i].data2);
		}
	}

}

BOOST_AUTO_TEST_SUITE_END()
