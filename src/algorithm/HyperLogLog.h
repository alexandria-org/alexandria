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

#include <cmath>
#include <cstring>
#include <algorithm>
#include <iostream>

namespace Algorithm {

	/*
	 * Implementation of the hyper log log algorithm as described by Flajolet1 et al.
	 * http://algo.inria.fr/flajolet/Publications/FlFuGaMe07.pdf
	 *
	 * Using 64 bit hash instead of 32bit.
	 * */

	template<typename T>
	class HyperLogLog {

		public:
			HyperLogLog();
			HyperLogLog(const HyperLogLog &other);
			HyperLogLog(const char *b);
			HyperLogLog(size_t b);
			~HyperLogLog();
			void insert(T v);
			void insert_hash(size_t x);
			size_t size() const;
			char leading_zeros_plus_one(size_t x) const;
			size_t num_zero_registers() const;
			double error_bound() const;

			const char *data() const { return m_M; };
			char *data() { return m_M; };
			size_t data_size() const { return m_len; };

			HyperLogLog operator +(const HyperLogLog &hl) const;
			HyperLogLog &operator +=(const HyperLogLog &hl);
			HyperLogLog &operator =(const HyperLogLog &other);

		private:
			
			char *m_M; // Points to registers.
			const int m_b = 15;
			const size_t m_len = 1ull << m_b; // 2^m_b
			const double m_alpha = 0.7213/(1.0 + 1.079/m_len);
			std::hash<std::string> m_hasher;

	};

	template<typename T>
	HyperLogLog<T>::HyperLogLog() {
		m_M = new char[m_len];
		memset(m_M, 0, m_len);
	}

	template<typename T>
	HyperLogLog<T>::HyperLogLog(const HyperLogLog<T> &other) {
		m_M = new char[m_len];
		memcpy(m_M, other.m_M, m_len);
	}

	template<typename T>
	HyperLogLog<T>::HyperLogLog(const char *m) {
		m_M = new char[m_len];
		memcpy(m_M, m, m_len);
	}

	template<typename T>
	HyperLogLog<T>::HyperLogLog(size_t b)
	: m_b(20) {
		m_M = new char[m_len];
		memset(m_M, 0, m_len);
	}

	template<typename T>
	HyperLogLog<T>::~HyperLogLog() {
		delete m_M;
	}

	template<typename T>
	void HyperLogLog<T>::insert(T v) {
		size_t x = m_hasher(std::to_string(v));
		size_t j = x >> (64-m_b);
		m_M[j] = std::max(m_M[j], leading_zeros_plus_one(x << m_b));
	}

	template<typename T>
	void HyperLogLog<T>::insert_hash(size_t x) {
		size_t j = x >> (64-m_b);
		m_M[j] = std::max(m_M[j], leading_zeros_plus_one(x << m_b));
	}

	template<typename T>
	size_t HyperLogLog<T>::size() const {
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

	template<typename T>
	char HyperLogLog<T>::leading_zeros_plus_one(size_t x) const {
		size_t num_zeros = 1;
		for (size_t i = 0; i < 64; i++) {
			if ((x >> (64 - 1 - i)) & 0x1ull) return num_zeros;
			num_zeros++;
		}
		return num_zeros;
	}

	template<typename T>
	size_t HyperLogLog<T>::num_zero_registers() const {
		size_t num_zero = 0;
		for (size_t i = 0; i < m_len; i++) {
			if (m_M[i] == 0) num_zero++;
		}
		return num_zero;
	}

	template<typename T>
	double HyperLogLog<T>::error_bound() const {
		double stdd = 1.04 / sqrt((double)m_len);
		return stdd * 3; // Gives 99% confidence
	}

	template<typename T>
	HyperLogLog<T> HyperLogLog<T>::operator +(const HyperLogLog<T> &hl) const {
		HyperLogLog res;
		for (size_t i = 0; i < m_len && i < hl.m_len; i++) {
			res.m_M[i] = std::max(m_M[i], hl.m_M[i]);
		}

		return res;
	}

	template<typename T>
	HyperLogLog<T> &HyperLogLog<T>::operator +=(const HyperLogLog<T> &hl) {
		for (size_t i = 0; i < m_len && i < hl.m_len; i++) {
			m_M[i] = std::max(m_M[i], hl.m_M[i]);
		}
		return *this;
	}

	template<typename T>
	HyperLogLog<T> &HyperLogLog<T>::operator =(const HyperLogLog<T> &other) {
		memcpy(m_M, other.m_M, m_len);
		return *this;
	}

}
