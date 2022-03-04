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
				intersection.emplace_back(generic_record(
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

	domain_level::domain_level() {
		m_builder = std::make_shared<index_builder<domain_record>>("domain", 0);
	}

	level_type domain_level::get_type() const {
		return level_type::domain;
	}

	void domain_level::add_snippet(const snippet &s) {
		for (size_t token : s.tokens()) {
			m_builder->add(token, domain_record(s.domain_hash()));
		}
	}

	void domain_level::add_document(size_t id, const string &doc) {
		std::vector<std::string> words = Text::get_full_text_words(doc);
		for (std::string &word : words) {
			m_builder->add(Hash::str(word), domain_record(id));
		}
	}

	void domain_level::add_index_file(const std::string &local_path,
		std::function<void(uint64_t, const std::string &)> callback) {

		const vector<size_t> cols = {1, 2, 3, 4};
		const vector<float> scores = {10.0, 3.0, 2.0, 1};

		ifstream infile(local_path, ios::in);
		string line;
		while (getline(infile, line)) {
			vector<string> col_values;
			boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

			URL url(col_values[0]);

			uint64_t domain_hash = url.host_hash();

			const string site_colon = "site:" + url.host() + " site:www." + url.host() + " " + url.host() + " " + url.domain_without_tld();

			for (size_t col : cols) {
				vector<string> words = Text::get_full_text_words(col_values[col]);
				for (const string &word : words) {
					m_builder->add(Hash::str(word), domain_record(domain_hash));
				}
			}
		}
	}

	void domain_level::merge() {
		m_builder->append();
		m_builder->merge();

		m_builder->calculate_scores(indexer::algorithm::bm25);
	}

	std::vector<return_record> domain_level::find(const string &query, const std::vector<size_t> &keys) {
		std::vector<std::string> words = Text::get_full_text_words(query);
		
		index<domain_record> idx("domain", 0);

		std::vector<std::vector<domain_record>> results;
		for (const string &word : words) {
			size_t token = Hash::str(word);
			results.push_back(idx.find(token));
		}
		std::vector<return_record> intersected = intersection(results);
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
			m_builders[dom_hash]->add(token, url_record(s.url_hash()));
		}
	}

	void url_level::add_document(size_t id, const string &doc) {
		
	}

	void url_level::add_index_file(const std::string &local_path,
		std::function<void(uint64_t, const std::string &)> callback) {
		const vector<size_t> cols = {1, 2, 3, 4};

		ifstream infile(local_path, ios::in);
		string line;
		while (getline(infile, line)) {
			vector<string> col_values;
			boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

			URL url(col_values[0]);

			uint64_t domain_hash = url.host_hash();
			uint64_t url_hash = url.hash();

			callback(url_hash, col_values[0] + "\t" + col_values[1]);

			if (m_builders.count(domain_hash) == 0) {
				m_builders[domain_hash] = std::make_shared<index_builder<url_record>>("url", domain_hash);
			}

			for (size_t col : cols) {
				vector<string> words = Text::get_full_text_words(col_values[col]);
				for (const string &word : words) {
					m_builders[domain_hash]->add(Hash::str(word), url_record(url_hash));
				}
			}
		}
	}

	void url_level::merge() {
		for (auto &iter : m_builders) {
			iter.second->append();
			iter.second->merge();

			iter.second->calculate_scores(algorithm::bm25);
		}
	}

	std::vector<return_record> url_level::find(const string &query, const std::vector<size_t> &keys) {
		std::vector<std::string> words = Text::get_full_text_words(query);
		std::vector<return_record> all_results;
		for (size_t key : keys) {
			index<url_record> idx("url", key);

			std::vector<std::vector<url_record>> results;
			for (const string &word : words) {
				size_t token = Hash::str(word);
				results.push_back(idx.find(token));
			}
			std::vector<return_record> intersected = intersection(results);
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
			m_builders[url_hash]->add(token, snippet_record(s.snippet_hash()));
		}
	}

	void snippet_level::add_document(size_t id, const string &doc) {
	}

	void snippet_level::add_index_file(const std::string &local_path,
		std::function<void(uint64_t, const std::string &)> callback) {

		ifstream infile(local_path, ios::in);
		string line;
		while (getline(infile, line)) {
			vector<string> col_values;
			boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

			URL url(col_values[0]);

			uint64_t url_hash = url.hash();

			if (m_builders.count(url_hash) == 0) {
				m_builders[url_hash] = std::make_shared<index_builder<snippet_record>>("snippet", url_hash, 0);
			}

			std::vector<std::string> snippets = Text::get_snippets(col_values[4]);

			size_t snippet_idx = 0;
			for (const std::string &snippet : snippets) {
				const size_t snippet_hash = (url_hash << 10) | snippet_idx;
				callback(snippet_hash, snippet);
				vector<uint64_t> tokens = Text::get_tokens(snippet);
				for (const uint64_t &token : tokens) {
					const size_t snippet_hash = (url_hash << 10) | snippet_idx;
					m_builders[url_hash]->add(token, snippet_record(snippet_hash));
				}
				snippet_idx++;
			}
		}
	}

	void snippet_level::merge() {
		for (auto &iter : m_builders) {
			iter.second->append();
			iter.second->merge();

			iter.second->calculate_scores(algorithm::bm25);
		}
	}

	std::vector<return_record> snippet_level::find(const string &query, const std::vector<size_t> &keys) {
		std::vector<std::string> words = Text::get_full_text_words(query);
		std::vector<return_record> all_results;
		for (size_t key : keys) {
			index<snippet_record> idx("snippet", key, 0);

			std::vector<std::vector<snippet_record>> results;
			for (const string &word : words) {
				size_t token = Hash::str(word);
				results.push_back(idx.find(token));
			}
			std::vector<return_record> summed_results = summed_union(results);
			sort_and_get_top_results(summed_results, 2); // Pick top 2 snippets.

			for (return_record &rec : summed_results) {
				rec.m_url_hash = key;
			}

			std::cout << "level: " << "snippet" << " found: " << summed_results.size() << " keys" << std::endl;
			all_results.insert(all_results.end(), summed_results.begin(), summed_results.end());
		}
		return all_results;
	}

}
