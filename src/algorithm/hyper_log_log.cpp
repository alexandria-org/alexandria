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

#include "hyper_log_log.h"

namespace algorithm {

	hyper_log_log::hyper_log_log() {
		m_M = new char[m_len];
		memset(m_M, 0, m_len);
	}

	hyper_log_log::hyper_log_log(const hyper_log_log &other) {
		m_M = new char[m_len];
		memcpy(m_M, other.m_M, m_len);
	}

	hyper_log_log::hyper_log_log(const char *m) {
		m_M = new char[m_len];
		memcpy(m_M, m, m_len);
	}

	hyper_log_log::hyper_log_log(size_t b)
	: m_b(b) {
		m_M = new char[m_len];
		memset(m_M, 0, m_len);
	}

	hyper_log_log::~hyper_log_log() {
		delete [] m_M;
	}

	void hyper_log_log::insert(size_t v) {
		size_t x = m_hasher(std::to_string(v));
		size_t j = x >> (64-m_b);
		m_M[j] = std::max(m_M[j], leading_zeros_plus_one(x << m_b));
	}

	size_t hyper_log_log::count() const {
		double Z = 0.0;
		for (size_t j = 0; j < m_len; j++) {
			Z += 1.0 / (1ull << m_M[j]);
		}
		double E = m_alpha * m_len * m_len / Z;

		// Only small range correction implemented since we use 64 bit hash.
		if (E <= (5.0/2.0) * m_len) {
			size_t V = num_zero_registers();
			if (V != 0) {
				E = m_len * log((double)m_len / V);
			}
		}

		return (size_t)E;
	}

	void hyper_log_log::reset() {
		memset(m_M, 0, m_len);
	}

	char hyper_log_log::leading_zeros_plus_one(size_t x) const {
		size_t num_zeros = 1;
		for (size_t i = 0; i < 64; i++) {
			if ((x >> (64 - 1 - i)) & 0x1ull) return num_zeros;
			num_zeros++;
		}
		return num_zeros;
	}

	size_t hyper_log_log::num_zero_registers() const {
		size_t num_zero = 0;
		for (size_t i = 0; i < m_len; i++) {
			if (m_M[i] == 0) num_zero++;
		}
		return num_zero;
	}

	double hyper_log_log::error_bound() const {
		double stdd = 1.04 / sqrt((double)m_len);
		return stdd * 3; // Gives 99% confidence
	}

	hyper_log_log hyper_log_log::operator +(const hyper_log_log &hl) const {
		hyper_log_log res;
		for (size_t i = 0; i < m_len && i < hl.m_len; i++) {
			res.m_M[i] = std::max(m_M[i], hl.m_M[i]);
		}

		return res;
	}

	hyper_log_log &hyper_log_log::operator +=(const hyper_log_log &hl) {
		for (size_t i = 0; i < m_len && i < hl.m_len; i++) {
			m_M[i] = std::max(m_M[i], hl.m_M[i]);
		}
		return *this;
	}

	hyper_log_log &hyper_log_log::operator =(const hyper_log_log &other) {
		memcpy(m_M, other.m_M, m_len);
		return *this;
	}

}