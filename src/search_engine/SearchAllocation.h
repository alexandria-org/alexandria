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

#include "full_text/FullTextResultSet.h"
#include "full_text/FullTextRecord.h"
#include "link_index/LinkFullTextRecord.h"
#include "link_index/DomainLinkFullTextRecord.h"
#include "config.h"
#include <map>
#include <vector>

using namespace std;

namespace SearchAllocation {

	/*
		The idea with this namespace is to handle all the memory allocation needed for serving a request to the search engine.
	*/

	template <typename DataRecord>
	struct Storage {
		/*
			result_sets holds pre-allocated object of class FullTextResultSet.
			result_sets[0 ... Config::ft_num_partitions][0 ... Config::query_max_words]
			first element in the map contains the partition number and the second is the index of the queried word
		*/
		map<size_t, vector<FullTextResultSet<DataRecord> *>> result_sets;

		// To hold the intersection of the result sets.
		map<size_t, FullTextResultSet<DataRecord> *> intersected_result;
	};

	struct Allocation {
		Storage<FullTextRecord> *storage;
		Storage<LinkFullTextRecord> *link_storage;
		Storage<DomainLinkFullTextRecord> *domain_link_storage;
	};

	template <typename DataRecord>
	Storage<DataRecord> *create_storage() {
		Storage<DataRecord> *storage = new Storage<DataRecord>;

		// Allocate result_sets.
		for (size_t i = 0; i < Config::ft_num_partitions; i++) {
			storage->result_sets[i].resize(Config::query_max_words, NULL);
			for (size_t j = 0; j < Config::query_max_words; j++) {
				storage->result_sets[i][j] = new FullTextResultSet<DataRecord>(Config::ft_max_results_per_section * Config::ft_max_sections);
			}
			storage->intersected_result[i] = new FullTextResultSet<DataRecord>(Config::ft_max_results_per_section * Config::ft_max_sections);
		}

		return storage;
	}

	template <typename DataRecord>
	void delete_storage(Storage<DataRecord> *storage) {

		// delete result_sets.
		for (size_t i = 0; i < Config::ft_num_partitions; i++) {
			for (size_t j = 0; j < Config::query_max_words; j++) {
				delete storage->result_sets[i][j];
			}
			delete storage->intersected_result[i];
		}

		delete storage;
	}

	Allocation *create_allocation();
	void delete_allocation(Allocation *allocation);

}
