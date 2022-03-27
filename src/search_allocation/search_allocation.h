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

#include "full_text/full_text_result_set.h"
#include "full_text/full_text_record.h"
#include "url_link/full_text_record.h"
#include "domain_link/full_text_record.h"
#include "config.h"
#include <map>
#include <vector>

namespace search_allocation {

	/*
		The idea with this namespace is to handle all the memory allocation needed for serving a request to the search engine.
	*/

	template <typename data_record>
	struct storage {
		/*
			result_sets holds pre-allocated object of class full_text_result_set.
			result_sets[0 ... Config::query_max_words]
		*/
		std::vector<full_text::full_text_result_set<data_record> *> result_sets;

		// To hold the intersection of the result sets.
		full_text::full_text_result_set<data_record> * intersected_result;
	};

	struct allocation {
		storage<full_text::full_text_record> *record_storage;
		storage<url_link::full_text_record> *link_storage;
		storage<domain_link::full_text_record> *domain_link_storage;
	};

	template <typename data_record>
	storage<data_record> *create_storage() {
		storage<data_record> *record_storage = new storage<data_record>;

		// Allocate result_sets.
		for (size_t j = 0; j < Config::query_max_words; j++) {
			record_storage->result_sets.push_back(new full_text::full_text_result_set<data_record>(Config::ft_max_results_per_section * Config::ft_max_sections));
		}
		record_storage->intersected_result = new full_text::full_text_result_set<data_record>(Config::ft_max_results_per_section * Config::ft_max_sections);

		return record_storage;
	}

	template <typename data_record>
	void delete_storage(storage<data_record> *storage) {

		// delete result_sets.
		for (size_t j = 0; j < Config::query_max_words; j++) {
			delete storage->result_sets[j];
		}
		delete storage->intersected_result;
		delete storage;
	}

	allocation *create_allocation();
	void delete_allocation(allocation *allocation);

}
