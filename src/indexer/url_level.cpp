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

#include "url_level.h"
#include "domain_stats/domain_stats.h"
#include "utils/thread_pool.hpp"

using namespace std;

namespace indexer {
	url_level::url_level() {
		clean_up();
	}

	level_type url_level::get_type() const {
		return level_type::url;
	}

	void url_level::add_snippet(const snippet &s) {
		(void)s;
	}

	void url_level::add_document(size_t id, const string &doc) {
		(void)id;
		(void)doc;
	}

	void url_level::add_index_file(const std::string &local_path,
		std::function<void(uint64_t, const std::string &)> add_data,
		std::function<void(uint64_t, uint64_t)> add_url) {

		(void)add_data;
		(void)add_url;

		const vector<size_t> cols = {1, 2, 3, 4};
		const vector<float> scores = {10.0, 3.0, 2.0, 1};

		ifstream infile(local_path, ios::in);
		string line;
		unordered_map<uint64_t, index_builder<url_record> *> builders;
		std::map<uint64_t, float> word_map;
		while (getline(infile, line)) {
			vector<string> col_values;
			boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

			URL url(col_values[0]);

			uint64_t domain_hash = url.host_hash();
			uint64_t url_hash = url.hash();

			if (builders.find(domain_hash) == builders.end()) {
				builders[domain_hash] = make_sure_builder_is_present(domain_hash);
			}
			index_builder<url_record> *builder = builders[domain_hash];

			(void)url_hash;
			(void)builder;

			url_record record(url_hash);
			record.url_length(url.path_with_query().size());

			size_t col_idx = 0;
			for (size_t col : cols) {
				auto tokens = text::get_tokens(col_values[col]);
				std::sort(tokens.begin(), tokens.end());
				auto last = std::unique(tokens.begin(), tokens.end());
				tokens.erase(last, tokens.end());
				for (auto token : tokens) {
					word_map[token] += scores[col_idx];
				}
			}

			for (const auto &iter : word_map) {
				record.m_score = iter.second;
				builder->add(iter.first, record);
			}

			word_map.clear();
		}
	}

	void url_level::add_link_file(const std::string &local_path, const ::algorithm::bloom_filter &url_filter) {

		profiler::instance prof("parsing " + local_path);
		ifstream infile(local_path, ios::in);
		string line;
		vector<string> col_values;
		set<uint64_t> tokens;
		unordered_map<uint64_t, index_builder<link_record> *> builders;
		size_t num_parsed = 0;
		size_t num_existed = 0;
		while (getline(infile, line)) {

			col_values.clear();
			boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

			URL target_url(col_values[2], col_values[3]);

			num_parsed++;
			if (!url_filter.exists(target_url.hash_input())) continue;
			num_existed++;

			URL source_url(col_values[0], col_values[1]);

			float source_harmonic = domain_stats::harmonic_centrality(source_url);

			const string link_text = col_values[4].substr(0, 1000);

			const uint64_t domain_hash = target_url.host_hash();
			const uint64_t link_hash = source_url.link_hash(target_url, link_text);

			if (builders.find(domain_hash) == builders.end()) {
				builders[domain_hash] = make_sure_link_builder_is_present(domain_hash);
			}
			index_builder<link_record> *builder = builders[domain_hash];

			vector<string> words = text::get_expanded_full_text_words(link_text);

			text::words_to_ngram_hash(words, 3, [&tokens](const uint64_t hash) {
				tokens.insert(hash);
			});

			// Add the url link.
			link_record link_rec(link_hash, source_harmonic);
			link_rec.m_source_domain = source_url.host_hash();
			link_rec.m_target_hash = target_url.hash();

			for (auto token : tokens) {
				builder->add(token, link_rec);
			}

			tokens.clear();
		}

		cout << "Done with " << local_path << " added " << num_existed << " total " << num_parsed << " took: " << prof.get() << "ms" << endl;
	}

	index_builder<url_record> *url_level::make_sure_builder_is_present(uint64_t domain_hash) {
		m_lock.lock();
		if (m_builders.count(domain_hash) == 0) {
			m_builders[domain_hash] = std::make_unique<index_builder<url_record>>("url", domain_hash, 1000);
		}

		auto ptr = m_builders[domain_hash].get();

		m_lock.unlock();

		return ptr;
	}

	index_builder<link_record> *url_level::make_sure_link_builder_is_present(uint64_t domain_hash) {
		m_lock.lock();
		if (m_link_builders.count(domain_hash) == 0) {
			m_link_builders[domain_hash] = std::make_unique<index_builder<link_record>>("url_links", domain_hash, 1000);
		}

		auto ptr = m_link_builders[domain_hash].get();

		m_lock.unlock();

		return ptr;
	}

	void url_level::merge() {

		// Just store the domain hashes in a file for later optimization.
		std::ofstream outfile("/root/all_domain_hashes.data", std::ios::binary | std::ios::app);
		for (const auto &iter : m_builders) {
			uint64_t domain_hash = iter.first;
			outfile.write((char *)&domain_hash, sizeof(uint64_t));
		}
		/*
		utils::thread_pool pool(32);
		for (const auto &iter : m_builders) {
			uint64_t domain_hash = iter.first;
			pool.enqueue([this, domain_hash]() {
				m_builders[domain_hash]->optimize();
			});
		}
		pool.run_all();*/
	}

	void url_level::clean_up() {
	}

	std::vector<return_record> url_level::find(size_t &total_num_results, const string &query, const std::vector<size_t> &keys,
		const vector<link_record> &links, const vector<domain_link_record> &domain_links, const std::vector<counted_record> &scores,
		const std::vector<domain_record> &domain_modifiers) {

		return {};
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
}
