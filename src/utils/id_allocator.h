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

#include <mutex>
#include <memory>

namespace utils {

	/*
	 * Very simple helper for allocating one shared object per id by multiple threads. Each thread should keep its own cache of the pointers since
	 * the get function locks execution.
	 *
	 *
	 * - thread A
	 *   std::unordered_map<uint64_t, data *> local_cache;
	 *   for (...) {
	 *		if (!local_cache.count(id)) {
	 *			local_cache[id] = alloc.get(id, ...); // alloc is shared instance of id_allocator
	 *		}
	 *
	 *		local_cache[id] can be used now.
	 *   }
	 * */

	template<typename alloc_type>
	class id_allocator {

		public:

			/*
			 * Allocates a pointer to an "alloc_type" object associated with id. The rest of the arguments are passed to the constructor of
			 * alloc_type.
			 * */
			template<class... type_args>
			alloc_type *get(uint64_t id, type_args&&... args) {

				std::lock_guard guard(m_lock);

				if (m_map.count(id) == 0) {
					m_map[id] = std::make_unique<alloc_type>(std::forward<type_args>(args)...);
				}

				return m_map[id].get();
			}

		private:

			std::mutex m_lock;
			std::unordered_map<uint64_t, std::unique_ptr<alloc_type>> m_map;

	};
	
}
