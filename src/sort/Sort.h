
#pragma once

#include <vector>

using namespace std;

namespace Sort {

	template<typename DataRecord, typename F>
	void merge_arrays(const vector<DataRecord> &arr1, const vector<DataRecord> &arr2, F compare, vector<DataRecord> &arr3) {

		int i = 0, j = 0;

		while (i < arr1.size() && j < arr2.size()) {
			if (compare(arr1[i], arr2[j])) {
				arr3.push_back(arr1[i++]);
			} else {
				arr3.push_back(arr2[j++]);
			}
		}

		while (i < arr1.size()) arr3.push_back(arr1[i++]);
		while (j < arr2.size()) arr3.push_back(arr2[j++]);
	}

	template<typename DataRecord>
	void merge_arrays(const vector<DataRecord> &arr1, const vector<DataRecord> &arr2, vector<DataRecord> &arr3) {
		merge_arrays(arr1, arr2, [](const DataRecord &a, const DataRecord &b) {
			return a < b;
		}, arr3);
	}

	template<typename DataRecord>
	void merge_arrays(const vector<vector<DataRecord>> &arrays, vector<DataRecord> &res) {
		merge_arrays(arrays, [](const DataRecord &a, const DataRecord &b) {
			return a < b;
		}, res);
	}

	template<typename DataRecord, typename F>
	void merge_array_range(const vector<vector<DataRecord>> &arrays, size_t i, size_t j, F compare, vector<DataRecord> &res) {
		if (i == j) {
			for (const DataRecord &rec : arrays[i]) {
				res.push_back(rec);
			}
		} else if (j - i == 1) {
			merge_arrays(arrays[i], arrays[j], compare, res);
		} else {
			vector<DataRecord> out1;
			vector<DataRecord> out2;

			merge_array_range(arrays, i, (i + j)/2, compare, out1);
			merge_array_range(arrays, (i + j)/2 + 1, j, compare, out2);

			merge_arrays(out1, out2, compare, res);
		}
	}

	template<typename DataRecord, typename F>
	void merge_arrays(const vector<vector<DataRecord>> &arrays, F compare, vector<DataRecord> &res) {
		if (arrays.size() == 0) return;
		merge_array_range(arrays, 0, arrays.size() - 1, compare, res);
	}

}
