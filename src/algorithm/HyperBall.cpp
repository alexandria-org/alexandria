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

#include "HyperBall.h"

#include "HyperLogLog.h"
#include "system/Profiler.h"

using namespace std;

namespace Algorithm {

	vector<double> hyper_ball(uint32_t n, const vector<uint32_t> *edge_map) {
		vector<HyperLogLog<uint32_t>> c(n);
		vector<HyperLogLog<uint32_t>> a(n);
		vector<double> harmonic(n, 0.0);

		for (uint32_t v = 0; v < n; v++) {
			c[v].insert(v);
		}

		size_t t = 0;
		Profiler::instance prof("Timetaker");
		while (true) {
			for (uint32_t v = 0; v < n; v++) {
				a[v] = c[v];
				for (const uint32_t &w : edge_map[v]) {
					a[v] += c[w];
				}

				// a[v] is t + 1 and c[v] is at t
				harmonic[v] += (1.0 / (t + 1)) * (a[v].size() - c[v].size());
				if (v % 1000000 == 0) {
					cout << fixed << "t = " << t << " got harmonic: " << harmonic[v] << " " << v << "/" << n << " in " << prof.get() << "ms (" << (double)v / ((double)prof.get() / 1000) << "/s)" << endl;
				}
			}
			for (uint32_t v = 0; v < n; v++) {
				c[v] = a[v];
			}
			cout << "Finished run t = " + to_string(t) << endl;
			t++;
			if (t > 20) break;
		}

		return harmonic;
	}

}
