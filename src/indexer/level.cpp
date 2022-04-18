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
#include "domain_stats/domain_stats.h"
#include "composite_index.h"
#include "sharded_index.h"

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
		clean_up();
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
		std::vector<std::string> words = text::get_full_text_words(doc);
		for (std::string &word : words) {
			m_builder->add(::algorithm::hash(word), domain_record(id));
		}
	}

	void domain_level::add_index_file(const std::string &local_path,
		std::function<void(uint64_t, const std::string &)> add_data,
		std::function<void(uint64_t, uint64_t)> add_url) {

		const vector<size_t> cols = {1, 2, 3, 4};
		const vector<float> scores = {10.0, 3.0, 2.0, 1};

		ifstream infile(local_path, ios::in);
		string line;
		while (getline(infile, line)) {
			vector<string> col_values;
			boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

			URL url(col_values[0]);

			uint64_t domain_hash = url.host_hash();
			float harmonic = domain_stats::harmonic_centrality(url);

			add_url(url.hash(), domain_hash);
			add_data(url.host_hash(), url.host());

			const string h = url.host();

			const string site_colon = "site:" + url.host() + " site:www." + url.host() + " " + url.host() + " " + url.domain_without_tld();

			for (size_t col : cols) {
				vector<string> words = text::get_full_text_words(col_values[col]);
				for (const string &word : words) {
					m_builder->add(::algorithm::hash(word), domain_record(domain_hash, harmonic));
				}
			}
		}
	}

	void domain_level::merge() {
		m_builder->append();
		m_builder->merge();
		m_builder->optimize();
	}

	void domain_level::calculate_scores() {
		m_builder->calculate_scores(indexer::algorithm::bm25);
	}

	void domain_level::clean_up() {
		m_builder = std::make_unique<sharded_index_builder<domain_record>>("domain", 1024);
		m_search_index = std::make_unique<sharded_index<domain_record>>("domain", 1024);
	}

	std::vector<return_record> domain_level::find(const string &query, const std::vector<size_t> &keys,
		const vector<link_record> &links, const vector<domain_link_record> &domain_links) {

		std::vector<std::string> words = text::get_full_text_words(query);
		std::vector<uint64_t> tokens(words.size());
		std::transform(words.begin(), words.end(), tokens.begin(), ::algorithm::hash);

		std::vector<domain_record> res = m_search_index->find_intersection(tokens);
		std::vector<return_record> intersected;
		for (const auto r : res) {
			intersected.emplace_back(return_record(r.m_value));
		}
		apply_domain_links(domain_links, intersected);
		sort_and_get_top_results(intersected, 100); // Pick top 100 domains.
		return intersected;
	}

	size_t domain_level::apply_domain_links(const vector<domain_link_record> &links, vector<return_record> &results) {
		if (links.size() == 0) return 0;

		size_t applied_links = 0;

		size_t i = 0;
		size_t j = 0;
		map<pair<uint64_t, uint64_t>, uint64_t> domain_unique;
		while (i < links.size() && j < results.size()) {

			const uint64_t hash1 = links[i].m_target_domain;
			const uint64_t hash2 = results[j].m_value;

			if (hash1 < hash2) {
				i++;
			} else if (hash1 == hash2) {
				auto p = std::make_pair(links[i].m_source_domain, links[i].m_target_domain);
				if (domain_unique.count(p) == 0) {
					const float url_score = expm1(25.0f*links[i].m_score) / 50.0f;
					results[j].m_score += url_score;
					results[j].m_num_domain_links++;
					applied_links++;
					domain_unique[p] = links[i].m_source_domain;
				}

				i++;
			} else {
				j++;
			}
		}

		return applied_links;
	}

	url_level::url_level() {
		clean_up();
	}

	level_type url_level::get_type() const {
		return level_type::url;
	}

	void url_level::add_snippet(const snippet &s) {
		size_t dom_hash = s.domain_hash();
		for (size_t token : s.tokens()) {
			m_builder->add(dom_hash, token, url_record(s.url_hash()));
		}
	}

	void url_level::add_document(size_t id, const string &doc) {
		
	}

	void url_level::add_index_file(const std::string &local_path,
		std::function<void(uint64_t, const std::string &)> add_data,
		std::function<void(uint64_t, uint64_t)> add_url) {
		const vector<size_t> cols = {1, 2, 3, 4};

		ifstream infile(local_path, ios::in);
		string line;
		while (getline(infile, line)) {
			vector<string> col_values;
			boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

			URL url(col_values[0]);

			uint64_t domain_hash = url.host_hash();
			uint64_t url_hash = url.hash();

			add_data(url_hash, col_values[0] + "\t" + col_values[1]);

			for (size_t col : cols) {
				vector<string> words = text::get_full_text_words(col_values[col]);
				for (const string &word : words) {
					m_builder->add(domain_hash, ::algorithm::hash(word), url_record(url_hash));
				}
			}
		}
	}

	void url_level::merge() {
		m_builder->append();
		m_builder->merge();
	}

	void url_level::clean_up() {
		m_builder = make_shared<composite_index_builder<url_record>>("url", 10007);
	}

	std::vector<return_record> url_level::find(const string &query, const std::vector<size_t> &keys,
		const vector<link_record> &links, const vector<domain_link_record> &domain_links) {

		std::vector<std::string> words = text::get_full_text_words(query);
		std::vector<return_record> all_results;
		for (size_t key : keys) {
			composite_index<url_record> idx("url", 10007);

			std::vector<std::vector<url_record>> results;
			for (const string &word : words) {
				size_t token = ::algorithm::hash(word);
				results.push_back(idx.find(key, token));
			}
			std::vector<return_record> intersected = intersection(results);
			apply_url_links(links, intersected);
			sort_and_get_top_results(intersected, 5); // Pick top 5 urls on each domain.
			all_results.insert(all_results.end(), intersected.begin(), intersected.end());
		}
		return all_results;
	}

	size_t url_level::apply_url_links(const vector<link_record> &links, vector<return_record> &results) {
		if (links.size() == 0) return 0;

		size_t applied_links = 0;

		size_t i = 0;
		size_t j = 0;
		map<pair<uint64_t, uint64_t>, uint64_t> domain_unique;
		while (i < links.size() && j < results.size()) {

			const uint64_t hash1 = links[i].m_target_hash;
			const uint64_t hash2 = results[j].m_value;

			if (hash1 < hash2) {
				i++;
			} else if (hash1 == hash2) {
				auto p = std::make_pair(links[i].m_source_domain, links[i].m_target_hash);
				if (domain_unique.count(p) == 0) {
					const float url_score = expm1(25.0f*links[i].m_score) / 50.0f;
					results[j].m_score += url_score;
					results[j].m_num_url_links++;
					applied_links++;
					domain_unique[p] = links[i].m_source_domain;
				}

				i++;
			} else {
				j++;
			}
		}

		return applied_links;
	}

	snippet_level::snippet_level() {
		clean_up();
	}

	level_type snippet_level::get_type() const {
		return level_type::snippet;
	}

	void snippet_level::add_snippet(const snippet &s) {
		size_t url_hash = s.url_hash();
		for (size_t token : s.tokens()) {
			m_builder->add(url_hash, token, snippet_record(s.snippet_hash()));
		}
	}

	void snippet_level::add_document(size_t id, const string &doc) {
	}

	void snippet_level::add_index_file(const std::string &local_path,
		std::function<void(uint64_t, const std::string &)> add_data,
		std::function<void(uint64_t, uint64_t)> add_url) {

		ifstream infile(local_path, ios::in);
		string line;
		while (getline(infile, line)) {
			vector<string> col_values;
			boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

			URL url(col_values[0]);

			uint64_t url_hash = url.hash();

			std::vector<std::string> snippets = text::get_snippets(col_values[4]);

			size_t snippet_idx = 0;
			for (const std::string &snippet : snippets) {
				const size_t snippet_hash = (url_hash << 10) | snippet_idx;
				add_data(snippet_hash, snippet);
				vector<uint64_t> tokens = text::get_tokens(snippet);
				for (const uint64_t &token : tokens) {
					const size_t snippet_hash = (url_hash << 10) | snippet_idx;
					m_builder->add(url_hash, token, snippet_record(snippet_hash));
				}
				snippet_idx++;
			}
		}
	}

	void snippet_level::merge() {
		m_builder->append();
		m_builder->merge();
	}

	void snippet_level::clean_up() {
		m_builder = make_shared<composite_index_builder<snippet_record>>("snippet", 10007);
	}

	std::vector<return_record> snippet_level::find(const string &query, const std::vector<size_t> &keys,
		const vector<link_record> &links, const vector<domain_link_record> &domain_links) {

		std::vector<std::string> words = text::get_full_text_words(query);
		std::vector<return_record> all_results;
		for (size_t key : keys) {
			composite_index<snippet_record> idx("snippet", 10007);

			std::vector<std::vector<snippet_record>> results;
			for (const string &word : words) {
				size_t token = ::algorithm::hash(word);
				results.push_back(idx.find(key, token));
			}
			std::vector<return_record> summed_results = summed_union(results);
			sort_and_get_top_results(summed_results, 2); // Pick top 2 snippets.

			for (return_record &rec : summed_results) {
				rec.m_url_hash = key;
			}

			all_results.insert(all_results.end(), summed_results.begin(), summed_results.end());
		}
		return all_results;
	}

}
