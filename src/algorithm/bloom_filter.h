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

#include <iostream>
#include <memory>

namespace algorithm {

	/*
	 * 2D bloom filter inspired by robustBF
	 *
	 * https://github.com/patgiri/robustBF/
	 * https://arxiv.org/abs/2106.04365
	 */
	class bloom_filter {
		public:
			bloom_filter();

			void insert(const std::string &item);
			bool exists(const std::string &item) const;
			size_t size() const { return m_dim_x * m_dim_y * sizeof(uint64_t); }
			const char *data() const { return (char *)m_bitmap.get(); }
			void read(char *data);

		private:

			std::unique_ptr<uint64_t[]> m_bitmap;

			size_t m_dim_x = 14009;
			size_t m_dim_y = 14071;

			std::array<uint64_t, 3> m_seeds = {7689571, 15485863, 98899};
			std::array<uint64_t, 3> m_hashes;

			void set_bit(size_t bit);
			bool get_bit(size_t bit) const;

	};

}
