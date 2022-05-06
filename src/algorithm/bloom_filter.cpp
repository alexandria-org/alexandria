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

#include "bloom_filter.h"
#include "algorithm/hash.h"
#include <cmath>
#include <cstring>

namespace algorithm {

	bloom_filter::bloom_filter()
	{
		m_bitmap = std::make_unique<uint64_t[]>(m_dim_x * m_dim_y);
	}

	void bloom_filter::insert(const std::string &item) {
		for (size_t i = 0; i < m_seeds.size(); i++) {
			uint64_t h = algorithm::hash_with_seed(item, m_seeds[i]);
			set_bit(h);
		}
	}

	bool bloom_filter::exists(const std::string &item) const {
		for (size_t i = 0; i < m_seeds.size(); i++) {
			uint64_t h = algorithm::hash_with_seed(item, m_seeds[i]);
			if (!get_bit(h)) return false;
		}
		return true;
	}

	void bloom_filter::read(char *data) {
		memcpy((char *)m_bitmap.get(), data, size());
	}

	void bloom_filter::set_bit(size_t bit) {
		const size_t x = bit % m_dim_x;
		const size_t y = bit % m_dim_y;
		const size_t pos = bit % 61;
		const size_t bitpos = y * m_dim_x + x;
		m_bitmap[bitpos] |= 0x1 << pos;
	}

	bool bloom_filter::get_bit(size_t bit) const {
		const size_t x = bit % m_dim_x;
		const size_t y = bit % m_dim_y;
		const size_t pos = bit % 61;
		const size_t bitpos = y * m_dim_x + x;
		return m_bitmap[bitpos] & (0x1 << pos);
	}

}
