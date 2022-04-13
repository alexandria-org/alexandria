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

#include "profiler.h"
#include "logger/logger.h"
#include <vector>
#include <map>

using namespace std;

namespace profiler {

	map<string, double> profiles_per_name;

	std::chrono::_V2::system_clock::time_point start_time = std::chrono::high_resolution_clock::now();

	instance::instance(const string &name) :
		m_name(name)
	{
		m_start_time = std::chrono::high_resolution_clock::now();
	}

	instance::instance() :
		m_name("unnamed profile")
	{
		m_start_time = std::chrono::high_resolution_clock::now();
	}

	instance::~instance() {
		profiles_per_name[m_name] += get();
		if (!m_has_stopped) {
			stop();
		}
	}

	void instance::enable() {
		m_enabled = true;
	}

	double instance::get() const {
		auto timer_elapsed = chrono::high_resolution_clock::now() - m_start_time;
		auto microseconds = chrono::duration_cast<std::chrono::microseconds>(timer_elapsed).count();

		return (double)microseconds/1000;
	}

	double instance::get_micro() const {
		if (!m_enabled) return 0;
		auto timer_elapsed = chrono::high_resolution_clock::now() - m_start_time;
		auto microseconds = chrono::duration_cast<std::chrono::microseconds>(timer_elapsed).count();

		return (double)microseconds;
	}

	void instance::stop() {
		if (!m_enabled) return;
		m_has_stopped = true;
		LOG_INFO("profiler [" + m_name + "] took " + to_string(get()) + "ms");
	}

	void instance::print() {
		if (!m_enabled) return;
		cout << "profiler [" + m_name + "] took " + to_string(get()) + "ms" << endl;
	}

	void print_memory_status() {
		ifstream infile("/proc/" + to_string(getpid()) + "/status");
		if (infile.is_open()) {
			string line;
			while (getline(infile, line)) {
				LOG_INFO(line);
			}
		}
	}

	void tick(const string &name, const string &section) {

	}
	void report_reset();
	void report_print();

	double now_micro() {
		auto timer_elapsed = chrono::high_resolution_clock::now() - start_time;
		auto microseconds = chrono::duration_cast<std::chrono::microseconds>(timer_elapsed).count();

		return (double)microseconds;
	}

	size_t timestamp() {
		const auto p1 = std::chrono::system_clock::now();
		return std::chrono::duration_cast<std::chrono::seconds>(p1.time_since_epoch()).count();
	}

	void print_report() {

		double total_ms = 0.0;
		for (const auto &iter : profiles_per_name) {
			total_ms += iter.second;
		}

		for (const auto &iter : profiles_per_name) {
			cout << iter.first << ": " << iter.second << "ms (" << 100.0 * (iter.second / total_ms) << "%)" << endl;
		}
	}

}

