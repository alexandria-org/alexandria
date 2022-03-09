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

#include <iostream>
#include <new>
#include <cstdlib>
#include <array>

using namespace std;

const bool debugger_verbose = false;
const int MAX_ALLOC_SIZE = 1000000;
std::array<void *, MAX_ALLOC_SIZE> alloc_ptrs{nullptr,};
std::array<size_t, MAX_ALLOC_SIZE> alloc_size{0,};
size_t alloc_counter = 0;

size_t total_allocated = 0;
size_t total_deleted = 0;

namespace memory {

	bool debugger_is_enabled = false;

	bool debugger_enabled() {
		return debugger_is_enabled;
	}

	void enable_debugger() {
		debugger_is_enabled = true;
	}

	size_t disable_debugger() {
		size_t num_unfreed = 0;
		if (debugger_verbose) {
			cout << "Unfreed pointers: " << endl;
		}
		for (auto i: alloc_ptrs){
			if (i != nullptr) {
				if (debugger_verbose) {
					cout << " " << (size_t)i << endl;
				}
				num_unfreed++;
			}
		}
		debugger_is_enabled = false;

		return num_unfreed;
	}

	size_t allocated_memory() {
		size_t allocated = 0;
		for (size_t i = 0; i < alloc_ptrs.size(); i++){
			if (alloc_ptrs[i] != nullptr) {
				allocated += alloc_size[i];
			}
		}

		return allocated;
	}
}

void * operator new(size_t n) {

	void *m = malloc(n);
	if (memory::debugger_enabled()) {
		total_allocated += n;
		if (alloc_counter < MAX_ALLOC_SIZE) {
			cout << "alloc_counter: " << alloc_counter << endl;
			alloc_ptrs[alloc_counter] = m;
			alloc_size[alloc_counter] = n;
			if (debugger_verbose) {
				cout << "new: " << (size_t)m << " with size " << n << " num: " << alloc_counter << endl;
			}
			alloc_counter++;
		}
	}
	return m;
}

void * operator new[](size_t n) {

	void *m = malloc(n);
	if (memory::debugger_enabled()) {
		total_allocated += n;
		if (alloc_counter < MAX_ALLOC_SIZE) {
			alloc_ptrs[alloc_counter] = m;
			alloc_size[alloc_counter] = n;
			if (debugger_verbose) {
				cout << "[new]: " << (size_t)m << " with size " << n << " num: " << alloc_counter << endl;
			}
			alloc_counter++;
		}
	}
	return m;
}

void operator delete(void *p) {
	if (memory::debugger_enabled()) {
		auto ind = std::distance(alloc_ptrs.begin(), std::find(alloc_ptrs.begin(), alloc_ptrs.end(), p));
		total_deleted += alloc_size[ind];
		alloc_ptrs[ind] = nullptr;
		if (debugger_verbose) {
			cout << "deleted: " << (size_t)p << " with size " << alloc_size[ind] << " mem allocated: " << (total_allocated - total_deleted) << endl;
		}
	}
	free(p);
}

void operator delete[](void *p) {
	if (memory::debugger_enabled()) {
		auto ind = std::distance(alloc_ptrs.begin(), std::find(alloc_ptrs.begin(), alloc_ptrs.end(), p));
		total_deleted += alloc_size[ind];
		alloc_ptrs[ind] = nullptr;
		if (debugger_verbose) {
			cout << "[deleted]: " << (size_t)p << " with size " << alloc_size[ind] << " mem allocated: " << (total_allocated - total_deleted) << endl;
		}
	}
	free(p);
}
