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

#include "utils/thread_pool.hpp"

BOOST_AUTO_TEST_SUITE(thread_pool)

BOOST_AUTO_TEST_CASE(thread_pool) {
	utils::thread_pool pool(10);

	vector<int> vec(10);
	for (int &i : vec) {
		pool.enqueue([&i]() {
			i++;
		});
	}

	pool.run_all();

	for (int i : vec) {
		BOOST_CHECK(i == 1);
	}
	
}

BOOST_AUTO_TEST_CASE(thread_pool2) {
	utils::thread_pool pool(12);

	vector<int> vec(24);
	for (int &i : vec) {
		pool.enqueue([&i]() {
				std::this_thread::sleep_for(200ms);
			i = 1;
		});
	}

	double now = Profiler::now_micro();

	pool.run_all();

	double dt = Profiler::now_micro() - now;

	BOOST_CHECK(dt < (200*2 + 10)*1000);

	for (int i : vec) {
		BOOST_CHECK(i == 1);
	}
	
}

BOOST_AUTO_TEST_SUITE_END()
