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

#include <vector>
#include <cstdint>
#include "hyper_log_log.h"
#include "profiler/profiler.h"
#include "logger/logger.h"
#include <future>

namespace algorithm {

	template <typename iterable_type>
	bool hyper_ball_worker(double t, size_t v_begin, size_t v_end, const iterable_type *edge_map,
			std::vector<hyper_log_log> &c, std::vector<hyper_log_log> &a, std::vector<double> &harmonic) {

		bool counter_changed = false;
		for (uint32_t v = v_begin; v < v_end; v++) {
			a[v] = c[v];
			for (const uint32_t &w : edge_map[v]) {
				a[v] += c[w];
			}

			// a[v] is t + 1 and c[v] is at t
			const size_t counter_diff = a[v].count() - c[v].count();
			if (counter_diff) {
				counter_changed = true;
				harmonic[v] += (1.0 / (t + 1.0)) * counter_diff;
			}
		}
		for (uint32_t v = v_begin; v < v_end; v++) {
			c[v] = a[v];
		}
		return counter_changed;
	}

	/*
	 * n is the number of vertices in graph.
	 * edge_map is pointing to an array of size n.
	 * each item in edge_map is a vector of variable size.
	 * each vector edge_map[m] contains values between 0 and n-1 indicating edge between m and edge_map[m].
	 * NOTE direction of edge in edge map has to be EDGE_FROM -> EDGE_TO.
	 * so for vertex m, n = edge_map[m] indicates directed edge from n to m
	 * */
	template <typename iterable_type>
	std::vector<double> hyper_ball(uint32_t n, const iterable_type *edge_map) {

		if (n == 0) return {};

		const size_t num_threads = std::min(32, (int)n);
		const size_t items_per_thread = n / num_threads;
		std::vector<hyper_log_log> c(n, hyper_log_log(10));
		std::vector<hyper_log_log> a(n, hyper_log_log(10));
		std::vector<double> harmonic(n, 0.0);

		for (uint32_t v = 0; v < n; v++) {
			c[v].insert(v);
		}

		double t = 0.0;
		while (true) {
			std::vector<std::future<bool>> threads;
			for (size_t i = 0; i < num_threads; i++) {
				const size_t v_begin = i * items_per_thread;
				const size_t v_end = (i == num_threads - 1) ? n : (i + 1) * items_per_thread;
				auto fut = std::async(hyper_ball_worker<iterable_type>, t, v_begin, v_end, edge_map, ref(c), ref(a), ref(harmonic));
				threads.emplace_back(std::move(fut));
			}

			bool should_continue = false;
			for (auto &fut : threads) {
				should_continue = fut.get() || should_continue;
			}

			t += 1.0;
			if (!should_continue) break;
		}

		return harmonic;
	}

}
