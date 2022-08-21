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

#include <boost/test/unit_test.hpp>
#include "utils/thread_pool.hpp"
#include "profiler/profiler.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(test_thread_pool)

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

	double now = profiler::now_micro();

	pool.run_all();

	double dt = profiler::now_micro() - now;

	BOOST_CHECK(dt < (200*2 + 10)*1000);

	for (int i : vec) {
		BOOST_CHECK(i == 1);
	}
	
}

/*
 * Test limit of queue length. The idea here is that if you pass a second parameter to the pool
 * you get a maximum queue length. Then if the workers are all working and the queue is full
 * the next call to enqueue will wait for the queue to become smaller.
 * 
 * This is useful if you want X workers to work but you don't want to fill up the queue because of.. limited memory.
 * */
BOOST_AUTO_TEST_CASE(thread_pool3) {
	utils::thread_pool pool(4, 1);

	vector<int> vec(4);
	int idx = 1;
	for (int &i : vec) {
		pool.enqueue([&i, idx]() {
			std::this_thread::sleep_for(200ms);
			i = idx;
		});
		// Allow some time for the work to be picked from the queue.
		std::this_thread::sleep_for(10ms);
		idx++;
	}
	// Now the 4 workers are working.

	// Enqueue one more.
	double now1 = profiler::now_micro();
	pool.enqueue([]() {
		std::this_thread::sleep_for(200ms);
	});
	double now2 = profiler::now_micro();

	// Should be quick.
	BOOST_CHECK(now2 - now1 < 10 * 1000); // < 10 milliseconds.

	// Now the next enqueue should wait around 200ms
	double now3 = profiler::now_micro();
	pool.enqueue([]() {
		std::this_thread::sleep_for(200ms);
	});
	double now4 = profiler::now_micro();

	BOOST_CHECK(now4 - now3 > 180 * 1000);

	std::this_thread::sleep_for(300ms);

	// All threads should be done now
	idx = 1;
	for (int i : vec) {
		BOOST_CHECK(i == idx);
		idx++;
	}

	pool.run_all();
}

BOOST_AUTO_TEST_SUITE_END()
