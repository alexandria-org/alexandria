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

#include "snippet.h"

using namespace std;

namespace indexer {

	std::string level_to_str(level_type lvl) {
		if (lvl == level_type::domain) return "domain";
		if (lvl == level_type::url) return "url";
		if (lvl == level_type::snippet) return "snippet";
		return "unknown";
	}

	template<typename data_record>
	std::vector<generic_record> level::intersection(const vector<vector<data_record>> &input) const {

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
		vector<generic_record> intersection;

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
				intersection.emplace_back(generic_record{
					.m_value = input[shortest_vector_position][positions[shortest_vector_position]].m_value,
					.m_score = score_sum / input.size()
				});
			}

			positions[shortest_vector_position]++;
		}

		return intersection;
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

	domain_level::domain_level() {
		m_builder = std::make_shared<index_builder<domain_record>>("domain", 0);
	}

	level_type domain_level::get_type() const {
		return level_type::domain;
	}

	void domain_level::add_snippet(const snippet &s) {
		for (size_t token : s.tokens()) {
			m_builder->add(token, domain_record{.m_value = s.domain_hash()});
		}
	}

	void domain_level::merge() {
		m_builder->append();
		m_builder->merge();
	}

	std::vector<generic_record> domain_level::find(const string &query, const std::vector<size_t> &keys) {
		std::vector<std::string> words = Text::get_full_text_words(query);
		
		index<domain_record> idx("domain", 0);

		std::vector<std::vector<domain_record>> results;
		for (const string &word : words) {
			size_t token = Hash::str(word);
			results.push_back(idx.find(token));
		}
		std::vector<generic_record> intersected = intersection(results);
		sort_and_get_top_results(intersected, 100); // Pick top 100 domains.
		std::cout << "level: " << "domain" << " found: " << intersected.size() << " keys" << std::endl;
		return intersected;
	}

	level_type url_level::get_type() const {
		return level_type::url;
	}

	void url_level::add_snippet(const snippet &s) {
		size_t dom_hash = s.domain_hash();
		if (m_builders.count(dom_hash) == 0) {
			m_builders[dom_hash] = std::make_shared<index_builder<url_record>>("url", dom_hash);
		}
		for (size_t token : s.tokens()) {
			m_builders[dom_hash]->add(token, url_record{.m_value = s.url_hash()});
		}
	}

	void url_level::merge() {
		for (auto &iter : m_builders) {
			iter.second->append();
			iter.second->merge();
		}
	}

	std::vector<generic_record> url_level::find(const string &query, const std::vector<size_t> &keys) {
		std::vector<std::string> words = Text::get_full_text_words(query);
		std::vector<generic_record> all_results;
		for (size_t key : keys) {
			index<url_record> idx("url", key);

			std::vector<std::vector<url_record>> results;
			for (const string &word : words) {
				size_t token = Hash::str(word);
				results.push_back(idx.find(token));
			}
			std::vector<generic_record> intersected = intersection(results);
			sort_and_get_top_results(intersected, 5); // Pick top 5 urls on each domain.
			std::cout << "level: " << "url" << " found: " << intersected.size() << " keys" << std::endl;
			all_results.insert(all_results.end(), intersected.begin(), intersected.end());
		}
		return all_results;
	}

	level_type snippet_level::get_type() const {
		return level_type::snippet;
	}

	void snippet_level::add_snippet(const snippet &s) {
		size_t url_hash = s.url_hash();
		if (m_builders.count(url_hash) == 0) {
			m_builders[url_hash] = std::make_shared<index_builder<snippet_record>>("snippet", url_hash, 0);
		}
		for (size_t token : s.tokens()) {
			m_builders[url_hash]->add(token, snippet_record{.m_value = s.snippet_hash()});
		}
	}

	void snippet_level::merge() {
		for (auto &iter : m_builders) {
			iter.second->append();
			iter.second->merge();
		}
	}

	std::vector<generic_record> snippet_level::find(const string &query, const std::vector<size_t> &keys) {
		std::vector<std::string> words = Text::get_full_text_words(query);
		std::vector<generic_record> all_results;
		for (size_t key : keys) {
			index<snippet_record> idx("snippet", key, 0);

			std::vector<std::vector<snippet_record>> results;
			for (const string &word : words) {
				size_t token = Hash::str(word);
				results.push_back(idx.find(token));
			}
			std::vector<generic_record> intersected = intersection(results);
			sort_and_get_top_results(intersected, 2); // Pick top 2 snippets.
			std::cout << "level: " << "snippet" << " found: " << intersected.size() << " keys" << std::endl;
			all_results.insert(all_results.end(), intersected.begin(), intersected.end());
		}
		return all_results;
	}

}
