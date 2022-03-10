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
#include <mutex>

using namespace std;

namespace memory {

	mutex lock;
	size_t mem_counter;
	size_t ptr_counter;

	bool debugger_enabled() {
		return false;
	}

	void enable_debugger() {

	}

	void enable_mem_counter() {

	}

	size_t disable_debugger() {
		return 0;
	}

	size_t allocated_memory() {
		return mem_counter;
	}

	size_t num_allocated() {
		return ptr_counter;
	}
}

//https://en.cppreference.com/w/cpp/memory/new/operator_new
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
