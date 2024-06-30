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
#include <mutex>
#include "roaring/roaring64map.hh"

namespace algorithm {

	class bloom_filter {
		public:
			bloom_filter();
			bloom_filter(size_t dim);

			void insert(const std::string &item);
			void insert(uint64_t item);
			void insert_many(std::vector<uint64_t> &items);
			bool exists(const std::string &item) const;
			bool exists(uint64_t data) const;
			size_t size() const { return m_dim * sizeof(uint64_t); }
			const char *data() const;
			void read(char *data, size_t len);
			void merge(const bloom_filter &other);
			double saturation();

			void read_file(const std::string &file_name);
			void write_file(const std::string &file_name) const;

		private:

			std::unique_ptr<uint64_t[]> m_bitmap;

			#ifdef IS_TEST
			size_t m_dim = 2695797;
			#else
			size_t m_dim = 2695797707;
			#endif

			size_t m_bitlen = m_dim * 64;

			// some random prime numbers
			std::array<uint64_t, 10> m_seeds = {3339675911, 2695798769, 2695831867, 2695857877, 2695879891, 2695879891, 2695922687, 2695935521,
					3339689791, 3339703163};

			std::mutex m_mutex;

			void set_bit(size_t bit);
			bool get_bit(size_t bit) const;

	};

}
