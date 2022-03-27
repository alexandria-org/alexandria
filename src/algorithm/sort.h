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

#pragma once

#include <vector>
#include <span>

namespace algorithm {

	namespace sort {

		template<typename data_record, typename F>
		void merge_arrays(const std::vector<data_record> &arr1, const std::vector<data_record> &arr2, F compare, std::vector<data_record> &arr3) {

			size_t i = 0, j = 0;

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

		template<typename data_record, typename F>
		void merge_arrays(const std::span<data_record> *arr1, const std::span<data_record> *arr2, F compare, std::vector<data_record> &arr3) {

			size_t i = 0, j = 0;

			while (i < arr1->size() && j < arr2->size()) {
				if (compare((*arr1)[i], (*arr2)[j])) {
					arr3.push_back((*arr1)[i++]);
				} else {
					arr3.push_back((*arr2)[j++]);
				}
			}

			while (i < arr1->size()) arr3.push_back((*arr1)[i++]);
			while (j < arr2->size()) arr3.push_back((*arr2)[j++]);
		}

		template<typename data_record>
		void merge_arrays(const std::vector<data_record> &arr1, const std::vector<data_record> &arr2, std::vector<data_record> &arr3) {
			merge_arrays(arr1, arr2, [](const data_record &a, const data_record &b) {
				return a < b;
			}, arr3);
		}

		template<typename data_record>
		void merge_arrays(const std::vector<std::vector<data_record>> &arrays, std::vector<data_record> &res) {
			merge_arrays(arrays, [](const data_record &a, const data_record &b) {
				return a < b;
			}, res);
		}

		template<typename data_record, typename F>
		void merge_array_range(const std::vector<std::vector<data_record>> &arrays, size_t i, size_t j, F compare, std::vector<data_record> &res) {
			if (i == j) {
				for (const data_record &rec : arrays[i]) {
					res.push_back(rec);
				}
			} else if (j - i == 1) {
				merge_arrays(arrays[i], arrays[j], compare, res);
			} else {
				std::vector<data_record> out1;
				std::vector<data_record> out2;

				merge_array_range(arrays, i, (i + j)/2, compare, out1);
				merge_array_range(arrays, (i + j)/2 + 1, j, compare, out2);

				merge_arrays(out1, out2, compare, res);
			}
		}

		template<typename data_record, typename F>
		void merge_arrays(const std::vector<std::vector<data_record>> &arrays, F compare, std::vector<data_record> &res) {
			if (arrays.size() == 0) return;
			merge_array_range(arrays, 0, arrays.size() - 1, compare, res);
		}

		template<typename data_record, typename F>
		void merge_array_range(const std::vector<std::span<data_record> *> &arrays, size_t i, size_t j, F compare, std::vector<data_record> &res) {
			if (i == j) {
				for (const data_record &rec : *(arrays[i])) {
					res.push_back(rec);
				}
			} else if (j - i == 1) {
				merge_arrays(arrays[i], arrays[j], compare, res);
			} else {
				std::vector<data_record> out1;
				std::vector<data_record> out2;

				merge_array_range(arrays, i, (i + j)/2, compare, out1);
				merge_array_range(arrays, (i + j)/2 + 1, j, compare, out2);

				merge_arrays(out1, out2, compare, res);
			}
		}

		template<typename data_record, typename F>
		void merge_arrays(const std::vector<std::span<data_record> *> &arrays, F compare, std::vector<data_record> &res) {
			if (arrays.size() == 0) return;
			merge_array_range(arrays, 0, arrays.size() - 1, compare, res);
		}
	
	}

}
