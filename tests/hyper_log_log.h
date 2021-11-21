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

#include "algorithm/HyperLogLog.h"
#include <cstdlib>

BOOST_AUTO_TEST_SUITE(hyper_log_log)

BOOST_AUTO_TEST_CASE(hyper_simple) {
	{
		Algorithm::HyperLogLog<uint32_t> hl;

		BOOST_CHECK(hl.leading_zeros_plus_one(0x0ull) == 65);
		BOOST_CHECK(hl.leading_zeros_plus_one(0x1ull) == 64);
		BOOST_CHECK(hl.leading_zeros_plus_one(0xFFFFFFFFull) == 33);
		BOOST_CHECK(hl.leading_zeros_plus_one(0xFFFFFFFFull) == 33);
	}
}

BOOST_AUTO_TEST_CASE(hyper_inserts) {

	vector<int> intervals = {400000, 500000, 1000000, 10000000};

	for (int interval : intervals) {
		Algorithm::HyperLogLog<uint32_t> hl;
		for (size_t i = 0; i < interval; i++) {
			hl.insert(i);
		}
		BOOST_CHECK(std::abs((int)hl.size() - interval) < interval * 0.0040625);
	}

}

BOOST_AUTO_TEST_CASE(hyper_union) {
	Algorithm::HyperLogLog<uint32_t> hl1;
	Algorithm::HyperLogLog<uint32_t> hl2;

	for (size_t i = 0; i < 250000; i++) {
		hl1.insert(i);
	}
	for (size_t i = 250000; i < 500000; i++) {
		hl2.insert(i);
	}

	Algorithm::HyperLogLog<uint32_t> hl3 = hl1 + hl2;
	BOOST_CHECK(std::abs((int)hl3.size() - 500000) < 500000 * 0.0040625);
}

BOOST_AUTO_TEST_SUITE_END();
