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

#include "hyper_ball.h"
#include "hyper_log_log.h"

#include "profiler/profiler.h"
#include "logger/logger.h"
#include <thread>

using namespace std;

namespace algorithm {

	void hyper_ball_worker(double t, size_t v_begin, size_t v_end, const vector<uint32_t> *edge_map,
			vector<hyper_log_log> &c, vector<hyper_log_log> &a, vector<double> &harmonic) {

		profiler::instance prof("Timetaker");
		for (uint32_t v = v_begin; v < v_end; v++) {
			a[v] = c[v];
			for (const uint32_t &w : edge_map[v]) {
				a[v] += c[w];
			}

			// a[v] is t + 1 and c[v] is at t
			harmonic[v] += (1.0 / (t + 1.0)) * (a[v].count() - c[v].count());
		}
		for (uint32_t v = v_begin; v < v_end; v++) {
			c[v] = a[v];
		}
	}

	vector<double> hyper_ball(uint32_t n, const vector<uint32_t> *edge_map) {

		const size_t num_threads = min(12, (int)n);
		const size_t items_per_thread = n / num_threads;
		vector<hyper_log_log> c(n);
		vector<hyper_log_log> a(n);
		vector<double> harmonic(n, 0.0);

		for (uint32_t v = 0; v < n; v++) {
			c[v].insert(v);
		}

		double t = 0.0;
		while (true) {
			vector<thread> threads;
			for (size_t i = 0; i < num_threads; i++) {
				const size_t v_begin = i * items_per_thread;
				const size_t v_end = (i == num_threads - 1) ? n : (i + 1) * items_per_thread;
				thread th(hyper_ball_worker, t, v_begin, v_end, edge_map, ref(c), ref(a), ref(harmonic));
				threads.push_back(move(th));
			}

			for (thread &th : threads) {
				th.join();
			}
			LOG_INFO("Finished run t = " + to_string(t));
			t += 1.0;
			if (t > 40.0) break;
		}

		return harmonic;
	}

}
