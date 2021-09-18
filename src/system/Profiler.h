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

#include <iostream>
#include <chrono>
#include <fstream>
#include <unistd.h>

#if __has_include("x86intrin.h")

	#define PROFILE_CPU_CYCLES true
	#include <x86intrin.h>

#else
	#define PROFILE_CPU_CYCLES false
#endif

using namespace std;

namespace Profiler {

	class instance {

	public:

		instance(const string &name);
		instance();
		~instance();

		void enable();
		double get() const;
		double get_micro() const;
		void stop();
		void print();

	private:
		string m_name;
		bool m_enabled = true;
		bool m_has_stopped = false;
		std::chrono::_V2::system_clock::time_point m_start_time;
	};

	void print_memory_status();
	uint64_t get_cycles();
	void measure_base_performance();
	double get_absolute_performance(double elapsed_ms);

}
