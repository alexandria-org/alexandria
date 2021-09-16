
#include "sort/Sort.h"

BOOST_AUTO_TEST_SUITE(sort)

struct TestDataStruct1 {
	int data1;
	int data2;
};

BOOST_AUTO_TEST_CASE(merge_arrays) {

	{
		vector<int> arr1 = {1, 2, 3};
		vector<int> arr2 = {4, 5, 6};
		vector<int> arr3;
		vector<int> arr4{1, 2, 3, 4, 5, 6};

		Sort::merge_arrays(arr1, arr2, arr3);

		BOOST_CHECK(arr3 == arr4);
	}

	{
		vector<int> arr1 = {1, 2, 3};
		vector<int> arr2 = {3, 4, 5, 6};
		vector<int> arr3;
		vector<int> arr4{1, 2, 3, 3, 4, 5, 6};

		Sort::merge_arrays(arr1, arr2, arr3);

		BOOST_CHECK(arr3 == arr4);
	}

	{
		vector<int> arr1 = {};
		vector<int> arr2 = {3, 4, 5, 6};
		vector<int> arr3;
		vector<int> arr4{3, 4, 5, 6};

		Sort::merge_arrays(arr1, arr2, arr3);

		BOOST_CHECK(arr3 == arr4);
	}

}

BOOST_AUTO_TEST_CASE(merge_arrays_of_struct) {

	{
		vector<struct TestDataStruct1> arr1{TestDataStruct1{.data1 = 1, .data2 = 2}};
		vector<struct TestDataStruct1> arr2{TestDataStruct1{.data1 = 2, .data2 = 3}};
		vector<struct TestDataStruct1> arr3;
		vector<struct TestDataStruct1> arr4{TestDataStruct1{.data1 = 1, .data2 = 2}, TestDataStruct1{.data1 = 2, .data2 = 3}};

		Sort::merge_arrays(arr1, arr2, [](const struct TestDataStruct1 &a, const struct TestDataStruct1 &b) {
			return a.data1 < b.data1;
		}, arr3);

		BOOST_CHECK(arr3[0].data1 == arr4[0].data1 && arr3[0].data2 == arr4[0].data2);
		BOOST_CHECK(arr3[1].data1 == arr4[1].data1 && arr3[1].data2 == arr4[1].data2);
	}

	{
		vector<struct TestDataStruct1> arr1{TestDataStruct1{.data1 = 1, .data2 = 2}, TestDataStruct1{.data1 = 3, .data2 = 4}};
		vector<struct TestDataStruct1> arr2{TestDataStruct1{.data1 = 2, .data2 = 3}};
		vector<struct TestDataStruct1> arr3;
		vector<struct TestDataStruct1> arr4{TestDataStruct1{.data1 = 1, .data2 = 2}, TestDataStruct1{.data1 = 2, .data2 = 3},
			TestDataStruct1{.data1 = 3, .data2 = 4}};

		Sort::merge_arrays(arr1, arr2, [](const struct TestDataStruct1 &a, const struct TestDataStruct1 &b) {
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

		Sort::merge_arrays(inp, res);

		BOOST_CHECK(res == corr);
	}

	{
		vector<int> arr1 = {1, 3, 6};
		vector<int> arr2 = {2, 4, 9};
		vector<int> arr3 = {1, 5, 7, 8};
		vector<int> res;
		vector<vector<int>> inp{arr1, arr2, arr3};
		vector<int> corr{1, 1, 2, 3, 4, 5, 6, 7, 8, 9};

		Sort::merge_arrays(inp, res);

		BOOST_CHECK(res == corr);
	}
}

BOOST_AUTO_TEST_CASE(merge_many_arrays_of_struct) {

	{
		vector<struct TestDataStruct1> arr1{
			TestDataStruct1{.data1 = 1, .data2 = 11},
			TestDataStruct1{.data1 = 2, .data2 = 12},
			TestDataStruct1{.data1 = 3, .data2 = 13}
		};
		vector<struct TestDataStruct1> arr2 = {
			TestDataStruct1{.data1 = 4, .data2 = 14},
			TestDataStruct1{.data1 = 5, .data2 = 15},
			TestDataStruct1{.data1 = 6, .data2 = 16}
		};
		vector<struct TestDataStruct1> arr3 = {
			TestDataStruct1{.data1 = 7, .data2 = 17},
			TestDataStruct1{.data1 = 8, .data2 = 18},
			TestDataStruct1{.data1 = 9, .data2 = 19}
		};
		vector<struct TestDataStruct1> res;
		vector<vector<struct TestDataStruct1>> inp{arr1, arr2, arr3};
		vector<struct TestDataStruct1> corr{
			TestDataStruct1{.data1 = 1, .data2 = 11},
			TestDataStruct1{.data1 = 2, .data2 = 12},
			TestDataStruct1{.data1 = 3, .data2 = 13},
			TestDataStruct1{.data1 = 4, .data2 = 14},
			TestDataStruct1{.data1 = 5, .data2 = 15},
			TestDataStruct1{.data1 = 6, .data2 = 16},
			TestDataStruct1{.data1 = 7, .data2 = 17},
			TestDataStruct1{.data1 = 8, .data2 = 18},
			TestDataStruct1{.data1 = 9, .data2 = 19}
		};

		Sort::merge_arrays(inp, [](const struct TestDataStruct1 &a, const struct TestDataStruct1 &b) {
			return a.data1 < b.data1;
		}, res);

		BOOST_CHECK(corr.size() == res.size());
		for (size_t i = 0; i < corr.size(); i++) {
			BOOST_CHECK(res[i].data1 == corr[i].data1 && res[i].data2 == corr[i].data2);
		}
	}

}

BOOST_AUTO_TEST_SUITE_END();
