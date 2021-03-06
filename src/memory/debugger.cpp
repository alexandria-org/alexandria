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

#include "debugger.h"
#include "memory.h"
#include "logger/logger.h"
#include <iostream>
#include <cstdlib>
#include <array>

using namespace std;

/*
	This memory manager exists so that we can know exactly how many bytes we currently have allocated. Since the OS
	is running a virtual memory system we can only know exactly how many bytes we have allocated right now if we
	keep a counter ourselves.

	To do this we overload the global new, new[], delete and delete[] operators.

	The problem is knowing how much memory is freed, we only have the pointer and not the length. To fix this issue
	we allocate sizeof(size_t) bytes more when allocating memory, then we store the length there and return a pointer
	to the address at offset "sizeof(size_t)" from the allocated pointer.

	This seems like absolute madness at first but I don't have any other solution.
*/

#include "sys/types.h"
#include "sys/sysinfo.h"

namespace memory {

	atomic_size_t mem_counter;
	size_t ptr_counter;
	size_t total_memory_on_host;

	void incr_mem_counter(size_t n) {
		mem_counter += n;
	}

	void decr_mem_counter(size_t n) {
		mem_counter -= n;
	}

	size_t allocated_memory() {
		return mem_counter;
	}

	size_t num_allocated() {
		return ptr_counter;
	}

	size_t record_usage_base = 0;
	size_t record_usage_peak = 0;
	size_t global_usage_peak = 0;

	void reset_usage() {
		record_usage_base = allocated_memory();
		record_usage_peak = record_usage_base;
	}

	void record_usage() {
		if (record_usage_peak < allocated_memory()) {
			record_usage_peak = allocated_memory();
		}
		if (global_usage_peak < get_usage()) {
			global_usage_peak = get_usage();
		}
	}

	size_t get_usage() {
		return record_usage_peak - record_usage_base;
	}

	size_t get_usage_peak() {
		return global_usage_peak;
	}

}

