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

#include "full_text/result_set.h"
#include "full_text/record.h"
#include "full_text/link_record.h"
#include "full_text/domain_link_record.h"
#include "config.h"
#include <map>
#include <vector>

namespace search_engine {

	/*
		The idea with this namespace is to handle all the memory allocation needed for serving a request to the search engine.
	*/

	template <typename data_record>
	struct storage {
		/*
			result_sets holds pre-allocated object of class full_text::result_set.
			result_sets[0 ... config::query_max_words]
		*/
		std::vector<std::unique_ptr<full_text::result_set<data_record>>> m_result_sets;

		// To hold the intersection of the result sets.
		std::unique_ptr<full_text::result_set<data_record>> m_intersected_result;
	};

	class allocation {

		public:

			allocation() {
				m_storage = create_storage();
				m_link_storage = std::make_unique();
				m_domain_link_storage = std::make_unique();
			}

		private:
			std::unique_ptr<storage<full_text::record>> m_storage;
			std::unique_ptr<storage<full_text::link_record>> m_link_storage;
			std::unique_ptr<storage<full_text::domain_link_record>> m_domain_link_storage;
	};

	template <typename data_record>
	std::unique_ptr<storage<data_record>> *create_storage() {
		auto storage = new Storage<data_record>;

		// Allocate result_sets.
		for (size_t j = 0; j < config::query_max_words; j++) {
			auto result_set = std::make_unique<full_text::result_set<data_record>>(config::ft_max_results_per_section * config::ft_max_sections);
			storage->result_sets.push_back(std::move(result_set));
		}
		storage->intersected_result = std::make_unique<full_text::result_set<data_record>>(config::ft_max_results_per_section * config::ft_max_sections);

		return storage;
	}

	allocation *create_allocation();
	void delete_allocation(allocation *allocation);

}
