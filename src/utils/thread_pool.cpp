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

#include "thread_pool.hpp"
#include <chrono>
#include <iostream>
#include <thread>
#include <future>
#include <queue>

using namespace std::chrono_literals;

namespace utils {

	thread_pool::thread_pool(size_t num_threads, size_t max_queue_len)
	: m_max_queue_len(max_queue_len) {
		for (size_t i = 0; i < num_threads; i++) {
			m_workers.emplace_back([this]() {
				this->handle_work();
			});
		}
	}
	thread_pool::~thread_pool() {
		run_all();
	}

	void thread_pool::enqueue(std::function<void()> &&fun) {

		if (m_stop) {
			throw std::runtime_error("enqueue on stopped thread_pool not allowed");
		}

		if (m_max_queue_len > 0) {
			while (true) {
				
				{
					std::lock_guard lock(m_queue_lock);
					if (m_queue.size() < m_max_queue_len) {
						m_queue.emplace(std::move(fun));
						break;
					}
				}
				std::this_thread::sleep_for(100ms);
			}
		} else {
			m_queue_lock.lock();
			m_queue.emplace(std::move(fun));
			m_queue_lock.unlock();
		}

		m_condition.notify_one();
	}

	void thread_pool::run_all() {
		if (m_stop) return; // Already stopped..
		m_queue_lock.lock();
		m_stop = true;
		m_queue_lock.unlock();
		m_condition.notify_all();

		for (std::thread &thread : m_workers) {
			if (thread.joinable()) {
				thread.join();
			}
		}
	}

	void thread_pool::handle_work() {
		while (true) {

			std::function<void()> task;

			{
				std::unique_lock<std::mutex> lock(m_queue_lock);
				m_condition.wait(lock, [this] {
					return m_stop || !m_queue.empty();
				});
				if (m_stop && m_queue.empty()) return;
				task = std::move(m_queue.front());
				m_queue.pop();
			}

			task();
		}
	}
	
}
