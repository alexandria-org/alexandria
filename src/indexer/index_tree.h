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

#include "index_builder.h"
#include "index.h"
#include "level.h"
#include "snippet.h"

namespace indexer {

	template<typename data_record>
	class index_tree {

	public:

		index_tree();
		~index_tree();

		void add_level(level_type lvl);
		void add_snippet(const snippet<data_record> &s);
		void merge();

		std::vector<data_record> find(const string &query);

	private:

		std::vector<level_type> m_levels;
		std::map<level_type, std::map<size_t, std::shared_ptr<index_builder<data_record>>>> m_indexes;

		std::shared_ptr<index_builder<data_record>> get_builder_at_level(level_type lvl, size_t key);
		std::vector<data_record> find_recursive(const string &query, size_t level_num, const std::vector<size_t> &keys);
		std::vector<data_record> intersection(const vector<vector<data_record>> &input);

	};

	template<typename data_record>
	index_tree<data_record>::index_tree() {
	}

	template<typename data_record>
	index_tree<data_record>::~index_tree() {
	}

	template<typename data_record>
	void index_tree<data_record>::add_level(level_type lvl) {
		m_levels.push_back(lvl);
	}

	template<typename data_record>
	void index_tree<data_record>::add_snippet(const snippet<data_record> &s) {
		size_t last_key = 0;
		for (level_type &lvl : m_levels) {
			auto builder = get_builder_at_level(lvl, last_key);
			
			size_t key = s.key(lvl);
			last_key = key;
			for (size_t &token : s.tokens()) {
				builder->add(token, data_record{.m_value = key, .m_score = 0.0f});
			}
		}
	}

	template<typename data_record>
	void index_tree<data_record>::merge() {
		for (auto &iter : m_indexes) {
			for (auto &iter : iter.second) {
				iter.second->append();
				iter.second->merge();
			}
		}
	}

	template<typename data_record>
	std::vector<data_record> index_tree<data_record>::find(const string &query) {
		return find_recursive(query, 0, {0});
	}

	template<typename data_record>
	std::shared_ptr<index_builder<data_record>> index_tree<data_record>::get_builder_at_level(level_type lvl, size_t key) {
		if (m_indexes[lvl].count(key) == 0) {
			// Allocate builder
			m_indexes[lvl][key] = std::make_shared<index_builder<data_record>>(std::to_string(lvl), key);
		}
		return m_indexes[lvl][key];
	}

	template<typename data_record>
	std::vector<data_record> index_tree<data_record>::find_recursive(const string &query, size_t level_num, const std::vector<size_t> &keys) {
		
		std::vector<std::string> words = Text::get_full_text_words(query);

		std::vector<data_record> all_results;
		for (size_t key : keys) {
			index<data_record> idx(std::to_string(m_levels[level_num]), key);

			std::vector<std::vector<data_record>> results;
			for (const string &word : words) {
				results.push_back(idx.find(Hash::str(word)));
			}
			std::vector<data_record> intersected = intersection(results);
			all_results.insert(all_results.end(), intersected.begin(), intersected.end());
		}
		
		if (level_num == m_levels.size() - 1) {
			// This is the last level, return the results instead of going deeper.
			return all_results;
		}
		// Go deeper. The m_value of results are keys for the next level...
		std::vector<size_t> next_level_keys;
		for (const data_record &rec : all_results) {
			next_level_keys.push_back(rec.m_value);
		}
		return find_recursive(query, level_num + 1, next_level_keys);
	}

	template<typename data_record>
	std::vector<data_record> index_tree<data_record>::intersection(const vector<vector<data_record>> &input) {

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
		vector<data_record> intersection;

		while (positions[shortest_vector_position] < shortest_len) {

			bool all_equal = true;
			data_record value = input[shortest_vector_position][positions[shortest_vector_position]];

			size_t iter_index = 0;
			for (const vector<data_record> &vec : input) {
				const size_t len = vec.size();

				size_t *pos = &(positions[iter_index]);
				while (*pos < len && value.m_value > vec[*pos].m_value) {
					(*pos)++;
				}
				if (((*pos < len) && (value.m_value < vec[*pos].m_value)) || *pos >= len) {
					all_equal = false;
					break;
				}
				iter_index++;
			}
			if (all_equal) {
				intersection.push_back(input[shortest_vector_position][positions[shortest_vector_position]]);
			}

			positions[shortest_vector_position]++;
		}

		return intersection;
	}

}
