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

#include "memory.h"
#include <unistd.h>
#include <iostream>
#include <fstream>

namespace memory {

	size_t available_memory = 0;
	size_t used_memory = 0;
	size_t total_memory = 0;

	size_t get_available_memory() {
		return available_memory;
	}

	size_t get_used_memory() {
		return used_memory;
	}

	size_t get_total_memory() {
		return total_memory;
	}

	/*
	 * inspired by https://stackoverflow.com/questions/349889/how-do-you-determine-the-amount-of-linux-system-ram-in-c
	 * */
	void update() {

		{
			std::string token;
			std::ifstream infile("/proc/meminfo", std::ios::in);
			if (infile.is_open()) {
				while (infile >> token) {
					if (token == "MemAvailable:") {
						size_t mem;
						if (infile >> mem) {
							available_memory = mem * 1000;
						} else {
							available_memory = 0;
						}
					}
					if (token == "MemTotal:") {
						size_t mem;
						if (infile >> mem) {
							total_memory = mem * 1000;
						} else {
							total_memory = 0;
						}
					}
				}
			}
		}

		{
			const size_t pid = getpid();
			std::string token;
			std::ifstream infile("/proc/" + std::to_string(pid) + "/stat", std::ios::in);
			if (infile.is_open()) {

				size_t counter = 1;
				while (infile >> token) {
					if (counter == 23) {
						used_memory = std::stoull(token);
						break;
					}
					counter++;
				}
			}
		}
	}

}
