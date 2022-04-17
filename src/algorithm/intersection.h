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
#include "roaring/roaring.hh"

namespace algorithm {

	roaring::Roaring intersection(const std::vector<roaring::Roaring> &input);

	template<typename item>
	std::vector<item> intersection(const std::vector<std::vector<item>> &input,
		std::function<void(item &a, const item &b)> sum_fun) {

		if (input.size() == 0) return {};

		size_t shortest_vector_position = 0;
		size_t shortest_len = SIZE_MAX;
		size_t iter_index = 0;
		for (const std::vector<item> &vec : input) {
			if (shortest_len > vec.size()) {
				shortest_len = vec.size();
				shortest_vector_position = iter_index;
			}
			iter_index++;
		}

		std::vector<size_t> positions(input.size(), 0);
		std::vector<item> intersection;

		while (positions[shortest_vector_position] < shortest_len) {

			bool all_equal = true;
			item value = input[shortest_vector_position][positions[shortest_vector_position]];

			size_t iter_index = 0;
			for (const std::vector<item> &vec : input) {
				const size_t len = vec.size();

				size_t *pos = &(positions[iter_index]);
				while (*pos < len && vec[*pos] < value) {
					(*pos)++;
				}
				if (((*pos < len) && (value < vec[*pos])) || *pos >= len) {
					all_equal = false;
					break;
				} else {
					if (iter_index != shortest_vector_position) {
						sum_fun(value, vec[*pos]);
					}
				}
				iter_index++;
			}
			if (all_equal) {
				intersection.push_back(value);
			}

			positions[shortest_vector_position]++;
		}

		return intersection;
	}

	template<typename item>
	std::vector<item> intersection(const std::vector<std::vector<item>> &input) {
		return intersection<item>(input, [](item &a, const item &b) {});
	}

}
