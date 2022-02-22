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

#include "index_tree.h"

using namespace std;

namespace indexer {

	index_tree::index_tree() {
	}

	index_tree::~index_tree() {
	}

	void index_tree::add_level(level *lvl) {
		create_directories(lvl->get_type());
		m_levels.push_back(lvl);
	}

	void index_tree::add_snippet(const snippet &s) {
		for (level *lvl : m_levels) {
			lvl->add_snippet(s);
		}
	}

	void index_tree::merge() {
		for (level *lvl : m_levels) {
			lvl->merge();
		}
	}

	void index_tree::truncate() {
		for (level *lvl : m_levels) {
			delete_directories(lvl->get_type());
			create_directories(lvl->get_type());
		}
	}

	std::vector<generic_record> index_tree::find(const string &query) {
		return find_recursive(query, 0, {0});
	}

	std::vector<generic_record> index_tree::find_recursive(const string &query, size_t level_num, const std::vector<size_t> &keys) {

		std::vector<generic_record> all_results = m_levels[level_num]->find(query, keys);
		
		if (level_num == m_levels.size() - 1) {
			// This is the last level, return the results instead of going deeper.
			return all_results;
		}
		// Go deeper. The m_value of results are keys for the next level...
		std::vector<size_t> next_level_keys;
		for (const generic_record &rec : all_results) {
			next_level_keys.push_back(rec.m_value);
		}
		return find_recursive(query, level_num + 1, next_level_keys);
	}

	void index_tree::create_directories(level_type lvl) {
		for (size_t i = 0; i < 8; i++) {
			boost::filesystem::create_directories("/mnt/" + std::to_string(i) + "/full_text/" + level_to_str(lvl));
		}
	}

	void index_tree::delete_directories(level_type lvl) {
		for (size_t i = 0; i < 8; i++) {
			boost::filesystem::remove_all("/mnt/" + std::to_string(i) + "/full_text/" + level_to_str(lvl));
		}
	}
}
