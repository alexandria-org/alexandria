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

namespace algorithm {

	/*
	 * Implementation of the hyper log log algorithm as described by Flajolet1 et al.
	 * http://algo.inria.fr/flajolet/Publications/FlFuGaMe07.pdf
	 *
	 * Using 64 bit hash instead of 32bit.
	 * */

	class hyper_log_log {

		public:
			hyper_log_log();
			hyper_log_log(const hyper_log_log &other);
			hyper_log_log(const char *b);
			hyper_log_log(size_t b);
			~hyper_log_log();

			void insert(size_t v);
			size_t count() const;
			double error_bound() const;
			void reset();

			const char *data() const { return m_M; };
			char *data() { return m_M; };
			size_t data_size() const { return m_len; };

			hyper_log_log operator +(const hyper_log_log &hl) const;
			hyper_log_log &operator +=(const hyper_log_log &hl);
			hyper_log_log &operator =(const hyper_log_log &other);

			char leading_zeros_plus_one(size_t x) const;

		private:
			
			char *m_M; // Points to registers.
			const int m_b = 15;
			const size_t m_len = 1ull << m_b; // 2^m_b
			const double m_alpha = 0.7213/(1.0 + 1.079/m_len);
			std::hash<std::string> m_hasher;

			size_t num_zero_registers() const;

	};

}
