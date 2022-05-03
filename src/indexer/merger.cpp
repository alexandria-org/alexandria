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

#include "merger.h"
#include "memory/memory.h"
#include "memory/debugger.h"
#include "utils/thread_pool.hpp"
#include <map>
#include <chrono>
#include <thread>

using namespace std;

namespace indexer {

	namespace merger {

		const double mem_limit = 0.5;

		bool is_merging = false;
		map<size_t, std::function<void()>> mergers;
		map<size_t, std::function<void()>> appenders;
		mutex merger_lock;

		void wait_for_merges() {
			while (is_merging) {
				std::this_thread::sleep_for(100ms);
			}
		}

		void lock() {
			if (is_merging) {
				wait_for_merges();
			}
		}

		void register_appender(size_t id, std::function<void()> append) {
			merger_lock.lock();
			appenders[id] = append;
			merger_lock.unlock();
		}

		void register_merger(size_t id, std::function<void()> merge) {
			merger_lock.lock();
			mergers[id] = merge;
			merger_lock.unlock();
		}

		void deregister_merger(size_t id) {
			merger_lock.lock();
			appenders.erase(id);
			mergers.erase(id);
			merger_lock.unlock();
		}

		bool merge_thread_is_running = true;
		thread merge_thread_obj;

		void append_all() {
			is_merging = true;
			this_thread::sleep_for(1000ms);

			size_t available_memory = memory::get_total_memory();

			std::cout << "APPENDING ALL: " << appenders.size() << " mergers allocated memory: " << memory::allocated_memory() << " limit is: " <<
				(available_memory * mem_limit) << std::endl;
			
			utils::thread_pool pool(32);

			merger_lock.lock();
			for (auto &iter : appenders) {
				pool.enqueue([iter]() {
					try {
						iter.second();
					} catch (...) {

					}
				});
			}

			pool.run_all();

			cout << "done... allocated memory: " << memory::allocated_memory() << endl;

			merger_lock.unlock();
			is_merging = false;
		}

		void merge_all() {
			is_merging = true;
			this_thread::sleep_for(1000ms);

			size_t available_memory = memory::get_total_memory();

			std::cout << "MERGING ALL: " << mergers.size() << " mergers allocated memory: " << memory::allocated_memory() << " limit is: " <<
				(available_memory * mem_limit) << std::endl;
			
			utils::thread_pool pool(32);

			for (auto &iter : mergers) {
				pool.enqueue([iter]() {
					try {
						iter.second();
					} catch (...) {
						
					}
				});
			}

			pool.run_all();

			cout << "done... allocated memory: " << memory::allocated_memory() << endl;

			is_merging = false;
		}

		void merge_thread() {
			memory::update();
			size_t available_memory = memory::get_total_memory();
			while (merge_thread_is_running) {
				if (memory::allocated_memory() > available_memory * mem_limit) {
					append_all();
				}
				this_thread::sleep_for(100ms);
			}
		}

		void start_merge_thread() {
			merge_thread_is_running = true;
			merge_thread_obj = std::move(thread(merge_thread));
		}

		void stop_merge_thread() {
			merge_thread_is_running = false;
			merge_thread_obj.join();
			append_all();
			merge_all();
		}

		void stop_merge_thread_only_append() {
			merge_thread_is_running = false;
			merge_thread_obj.join();
			append_all();
		}

		void terminate_merge_thread() {
			merge_thread_is_running = false;
			merge_thread_obj.join();
		}

		void force_append() {
			append_all();
		}
	}

}
