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

#include <iostream>
#include <memory>
#include <map>
#include <vector>
#include "snippet.h"
#include "index_builder.h"
#include "sharded_index_builder.h"
#include "sharded_index.h"
#include "index.h"
#include "generic_record.h"
#include "roaring/roaring.hh"
#include "counted_record.h"
#include "link_record.h"
#include "domain_link_record.h"
#include "domain_record.h"
#include "algorithm/bloom_filter.h"

namespace indexer {

	class snippet;

	enum class level_type { domain = 101, url = 102, snippet = 103 };

	std::string level_to_str(level_type lvl);

	/*
	This is the returned record from the index_manager. It contains more data than the stored record.
	*/
	class return_record : public generic_record {

		public:
		uint64_t m_url_hash;
		size_t m_num_url_links = 0;
		size_t m_num_domain_links = 0;

		return_record() : generic_record() {};
		return_record(uint64_t value) : generic_record(value) {};
		return_record(uint64_t value, float score) : generic_record(value, score) {};

	};

	class level {
		public:
		virtual level_type get_type() const = 0;
		virtual void add_snippet(const snippet &s) = 0;
		virtual void add_document(size_t id, const std::string &doc) = 0;
		virtual void add_index_file(const std::string &local_path,
			std::function<void(uint64_t, const std::string &)> add_data,
			std::function<void(uint64_t, uint64_t)> add_url) = 0;
		virtual void add_link_file(const std::string &local_path, const ::algorithm::bloom_filter &url_filter) = 0;
		virtual void merge() = 0;
		virtual void calculate_scores() = 0;
		virtual void clean_up() = 0;
		virtual std::vector<return_record> find(size_t &total_num_results, const std::string &query, const std::vector<size_t> &keys,
			const std::vector<link_record> &links, const std::vector<domain_link_record> &domain_links, const std::vector<counted_record> &scores,
			const std::vector<domain_record> &domain_modifiers) = 0;

		protected:
		template<typename data_record>
		roaring::Roaring intersection(const std::vector<roaring::Roaring> &input) const;
		template<typename data_record>
		std::vector<return_record> intersection(const std::vector<std::vector<data_record>> &input) const;

		template<typename data_record>
		std::vector<return_record> summed_union(const std::vector<std::vector<data_record>> &input) const;

		template<typename data_record>
		void sort_and_get_top_results(std::vector<data_record> &input, size_t num_results) const;

		mutex m_lock;
	};

	template<typename data_record>
	std::vector<return_record> level::intersection(const vector<vector<data_record>> &input) const {

		if (input.size() == 0) return {};

		size_t shortest_vector_position = 0;
		size_t shortest_len = SIZE_MAX;
		size_t iter_index = 0;
		for (const vector<data_record> &vec : input) {
			if (shortest_len > vec.size()) {
				shortest_len = vec.size();
				shortest_vector_position = iter_index;
			}
			iter_index++;
		}

		vector<size_t> positions(input.size(), 0);
		vector<return_record> intersection;

		while (positions[shortest_vector_position] < shortest_len) {

			bool all_equal = true;
			data_record value = input[shortest_vector_position][positions[shortest_vector_position]];

			float score_sum = 0.0f;
			size_t iter_index = 0;
			for (const vector<data_record> &vec : input) {
				const size_t len = vec.size();

				size_t *pos = &(positions[iter_index]);
				while (*pos < len && value.m_value > vec[*pos].m_value) {
					(*pos)++;
				}
				if (*pos < len && value.m_value == vec[*pos].m_value) {
					score_sum += vec[*pos].m_score;
				}
				if (((*pos < len) && (value.m_value < vec[*pos].m_value)) || *pos >= len) {
					all_equal = false;
					break;
				}
				iter_index++;
			}
			if (all_equal) {
				intersection.emplace_back(return_record(
					input[shortest_vector_position][positions[shortest_vector_position]].m_value,
					score_sum / input.size()
					));
			}

			positions[shortest_vector_position]++;
		}

		return intersection;
	}

	template<typename data_record>
	std::vector<return_record> level::summed_union(const vector<vector<data_record>> &input) const {
		vector<return_record> records;
		for (const vector<data_record> &vec : input) {
			for (const data_record &rec : vec) {
				records.push_back(return_record(rec.m_value, rec.m_score));
			}
		}
		sort(records.begin(), records.end());
		// Sum equal elements.
		for (size_t i = 0, j = 1; i < records.size() && j < records.size(); j++) {
			if (records[i] != records[j]) {
				i = j;
			} else {
				records[i] += records[j];
			}
		}
		// Delete consecutive elements. Only keeping the first.
		auto last = std::unique(records.begin(), records.end());
		records.erase(last, records.end());
		return records;		
	}

	template<typename data_record>
	void level::sort_and_get_top_results(std::vector<data_record> &input, size_t num_results) const {
		sort(input.begin(), input.end(), [](const data_record &a, const data_record &b) {
			return a.m_score > b.m_score;
		});
		if (input.size() > num_results) {
			input.resize(num_results);
		}
	}

}
