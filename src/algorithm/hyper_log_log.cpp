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

#include <numeric>
#include "hyper_log_log.h"
#include "algorithm/hash.h"

namespace algorithm {

	hyper_log_log::hyper_log_log(size_t b)
	: m_b(b), m_len(1ull << m_b), m_alpha(0.7213/(1.0 + 1.079/m_len)) {
		m_M.resize(m_len);
		std::fill(m_M.begin(), m_M.end(), 0);
	}

	hyper_log_log::hyper_log_log(const char *registers, size_t b)
	: m_b(b), m_len(1ull << m_b), m_alpha(0.7213/(1.0 + 1.079/m_len)) {
		m_M.resize(m_len);
		memcpy(m_M.data(), registers, m_len);
	}

	hyper_log_log::hyper_log_log(const hyper_log_log &other)
	: m_b(other.m_b), m_len(other.m_len), m_alpha(other.m_alpha) {
		m_M.resize(m_len);
		std::copy(other.m_M.cbegin(), other.m_M.cend(), m_M.begin());
	}

	hyper_log_log::hyper_log_log(hyper_log_log &&other)
	: m_b(other.m_b), m_len(other.m_len), m_alpha(other.m_alpha) {
		m_M.swap(other.m_M);
	}

	hyper_log_log::~hyper_log_log() {
	}

	void hyper_log_log::insert(size_t v) {
		size_t x = algorithm::hash(std::to_string(v));
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
		std::fill(m_M.begin(), m_M.end(), 0);
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
		return std::transform_reduce(m_M.begin(), m_M.end(), 0,
			[](int a, int b) { return a + b; },
			[](char a) { return a == 0 ? 1 : 0; });
	}

	double hyper_log_log::error_bound() const {
		double stdd = 1.04 / sqrt((double)m_len);
		return stdd * 3; // Gives 99% confidence
	}

	hyper_log_log hyper_log_log::operator +(const hyper_log_log &hl) const {
		hyper_log_log res;
		std::transform(std::begin(m_M), std::end(m_M), std::begin(hl.m_M), std::begin(res.m_M), [] (char a, char b) { return std::max(a, b); });

		return res;
	}

	hyper_log_log &hyper_log_log::operator +=(const hyper_log_log &hl) {
		std::transform(std::begin(m_M), std::end(m_M), std::begin(hl.m_M), std::begin(m_M), [] (char a, char b) { return std::max(a, b); });
		return *this;
	}

	hyper_log_log &hyper_log_log::operator =(const hyper_log_log &other) {
		std::copy(other.m_M.cbegin(), other.m_M.cend(), m_M.begin());
		return *this;
	}

}
