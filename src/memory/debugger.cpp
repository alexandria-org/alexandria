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
#include <iostream>
#include <new>
#include <cstdlib>
#include <array>
#include <mutex>

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

namespace memory {

	mutex lock;
	size_t mem_counter;
	size_t ptr_counter;

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

/*
	Overload the global new, new[], delete and delete[] operators.
*/
// https://en.cppreference.com/w/cpp/memory/new/operator_new
void *operator new(size_t n) {

	void *m = malloc(n + sizeof(size_t));

	if (m) {
		memory::lock.lock();
		memory::mem_counter += n;
		memory::ptr_counter++;
		memory::lock.unlock();
		static_cast<size_t *>(m)[0] = n;
		return &(static_cast<size_t *>(m)[1]);
	}

	throw bad_alloc();
}

void *operator new[](size_t n) {

	void *m = malloc(n + sizeof(size_t));

	if (m) {
		memory::lock.lock();
		memory::mem_counter += n;
		memory::ptr_counter++;
		memory::lock.unlock();
		static_cast<size_t *>(m)[0] = n;
		return &(static_cast<size_t *>(m)[1]);
	}

	throw bad_alloc();
}

void operator delete(void *p) noexcept {

	void *realp = &(static_cast<size_t *>(p)[-1]);
	const size_t n = static_cast<size_t *>(p)[-1];

	memory::lock.lock();
	memory::mem_counter -= n;
	memory::ptr_counter--;
	memory::lock.unlock();

	free(realp);
}

void operator delete[](void *p) noexcept {

	void *realp = &(static_cast<size_t *>(p)[-1]);
	const size_t n = static_cast<size_t *>(p)[-1];

	memory::lock.lock();
	memory::mem_counter -= n;
	memory::ptr_counter--;
	memory::lock.unlock();

	free(realp);
}
