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
#include <cmath>

namespace algorithm {

	bloom_filter::bloom_filter(size_t expected_num_elements, float p)
	: m_n(expected_num_elements), m_p(p)
	{
		m_num_bits = std::ceil((m_n * log(m_p)) / log(1.0 / pow(2.0, log(2.0))));
		m_num_bytes = std::ceil(m_num_bits / 8);
		m_k = std::round((m_num_bits / m_n) * log(2));

		m_bitmap = std::make_unique<char[]>(m_num_bytes);
	}

	void bloom_filter::insert(uint64_t item) {
		for (size_t i = 0; i < m_k; i++) {
			set_bit((item % (m_num_bits + i)) % m_num_bits);
		}
	}

	bool bloom_filter::exists(uint64_t item) const {
		for (size_t i = 0; i < m_k; i++) {
			if (!get_bit((item % (m_num_bits + i)) % m_num_bits)) return false;
		}
		return true;
	}

	void bloom_filter::set_bit(size_t bit) {
		m_bitmap[bit / 8] |= 0x1 << (bit % 8);
	}

	bool bloom_filter::get_bit(size_t bit) const {
		return m_bitmap[bit / 8] & (0x1 << (bit % 8));
	}

}
