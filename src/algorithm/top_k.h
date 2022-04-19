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
#include <functional>

namespace algorithm {

	/*
	 * Returns top k elements in unsorted const vector in linear time using a 2k memory buffer.
	 * */

	template<class dtype>
	std::vector<dtype> top_k(const std::vector<dtype> &input, size_t k,
		std::function<bool(const dtype &, const dtype &)> ordered) {
		
		if (input.size() <= k) return input;
		if (input.size() <= 2 * k) {
			std::vector<dtype> buf(input.begin(), input.end());
			std::nth_element(buf.begin(), buf.begin() + buf.size() / 2, buf.end(), ordered);
			return std::vector<dtype>(buf.begin() + buf.size() / 2, buf.end());
		}

		std::vector<dtype> buf(input.begin(), input.begin() + (2 * k));

		size_t idx = 2 * k;
		while (idx < input.size()) {
			std::nth_element(buf.begin(), buf.begin() + k, buf.end(), ordered);
			for (size_t i = 0, j = idx; i < k && j < input.size(); i++, j++) {
				// Only insert objects that are out of order compared to pivot buf[k]
				if (!ordered(input[j], buf[k])) {
					buf[i] = input[idx + i];
				}
			}
			idx += k;
		}
		// Run final partition.
		std::nth_element(buf.begin(), buf.begin() + buf.size() / 2, buf.end(), ordered);

		return std::vector<dtype>(buf.begin() + k, buf.end());
	}

	/*
	 * top_k but with default less than operator.
	 * */
	template<class dtype>
	std::vector<dtype> top_k(const std::vector<dtype> &input, size_t k) {
		return top_k<dtype>(input, k, [](const dtype &a, const dtype &b) { return a < b; });
	}

}
