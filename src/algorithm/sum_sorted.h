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

	template<class dtype>
	std::vector<dtype> sum_sorted(const std::vector<std::vector<dtype>> &input,
			std::function<void(dtype &a, const dtype &b)> plus_eq) {

		const size_t n = input.size();
		if (n == 0) return {};

		std::vector<dtype> ret;
		std::vector<size_t> pos(n, 0);
		
		while (true) {
			int start_vec = -1;
			for (size_t i = 0; i < n; i++) {
				if (pos[i] < input[i].size() ) {
					start_vec = i;
					break;
				}
			}
			if (start_vec == -1) break;

			dtype smallest = input[start_vec][pos[start_vec]];

			for (size_t i = 0; i < n; i++) {
				if (pos[i] < input[i].size() && input[i][pos[i]] < smallest) {
					smallest = input[i][pos[i]];
					start_vec = i;
				}
			}

			const dtype el = input[start_vec][pos[start_vec]];
			dtype sum = el;
			pos[start_vec]++;
			for (size_t i = start_vec + 1; i < n; i++) {
				while (pos[i] < input[i].size() && input[i][pos[i]] < el) {
					pos[i]++;
				}
				if (input[i][pos[i]] == el) {
					plus_eq(sum, input[i][pos[i]]);
					pos[i]++;
				}
			}
			ret.push_back(sum);
		}
		return ret;
	}

}
