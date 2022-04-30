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

#include "domain_level.h"
#include "snippet.h"
#include "domain_stats/domain_stats.h"
#include "composite_index.h"
#include "sharded_index.h"

using namespace std;

namespace indexer {

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
		const vector<link_record> &links, const vector<domain_link_record> &domain_links, const std::vector<counted_record> &scores) {

		(void)keys;
		(void)links;

		std::vector<std::string> words = text::get_full_text_words(query);
		std::vector<uint64_t> tokens(words.size());
		std::transform(words.begin(), words.end(), tokens.begin(), ::algorithm::hash);

		size_t dom_incr = 0;
		size_t score_incr = 0;
		auto score_mod = [&dom_incr, &domain_links, &score_incr, &scores](uint64_t value) {

			float score = 0.0f;
			while (score_incr < scores.size() && scores[score_incr].m_value < value) {
				score_incr++;
			}
			if (score_incr < scores.size() && scores[score_incr].m_value == value) {
				score += scores[score_incr].m_score;
			}

			while (dom_incr < domain_links.size() && domain_links[dom_incr].m_target_domain < value) {
				dom_incr++;
			}
			if (dom_incr < domain_links.size() && domain_links[dom_incr].m_target_domain == value) {
				score += domain_links[dom_incr].m_score;
			}
			return score;
		};

		std::vector<domain_record> res = m_search_index->find_top(tokens, score_mod, 20);
		std::vector<return_record> intersected;
		for (const auto r : res) {
			intersected.emplace_back(return_record(r.m_value, r.m_score));
		}
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

}
