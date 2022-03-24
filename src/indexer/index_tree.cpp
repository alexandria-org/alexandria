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
#include "merger.h"
#include "domain_stats/domain_stats.h"
#include "link/Link.h"
#include "algorithm/Algorithm.h"
#include "utils/thread_pool.hpp"

using namespace std;

namespace indexer {

	index_tree::index_tree() {
		m_link_index_builder = std::make_unique<sharded_index_builder<link_record>>("link_index", 2001);
		m_domain_link_index_builder = std::make_unique<sharded_index_builder<domain_link_record>>("domain_link_index", 2001);
		m_link_index = std::make_unique<sharded_index<link_record>>("link_index", 2001);
		m_domain_link_index = std::make_unique<sharded_index<domain_link_record>>("domain_link_index", 2001);
		m_hash_table = std::make_unique<hash_table::builder>("index_tree");
		m_url_to_domain = std::make_unique<UrlToDomain>("index_tree"); 
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

	void index_tree::add_document(size_t id, const string &doc) {
		for (level *lvl : m_levels) {
			lvl->add_document(id, doc);
		}
	}

	void index_tree::add_index_file(const string &local_path) {
		for (level *lvl : m_levels) {
			lvl->add_index_file(local_path, [this](uint64_t key, const string &value) {
				m_hash_table->add(key, value);
			}, [this](uint64_t url_hash, uint64_t domain_hash) {
				m_url_to_domain->add_url(url_hash, domain_hash);
			});
		}
	}

	void index_tree::add_index_files_threaded(const vector<string> &local_paths, size_t num_threads) {

		utils::thread_pool pool(num_threads);

		for (const string &local_path : local_paths) {
			pool.enqueue([this, local_path]() -> void {
				add_index_file(local_path);
			});
		}

		pool.run_all();

		m_url_to_domain->write(0);
		m_hash_table->merge();

	}

	void index_tree::add_link_file(const string &local_path) {

		ifstream infile(local_path, ios::in);
		string line;
		while (getline(infile, line)) {
			vector<string> col_values;
			boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

			URL target_url(col_values[2], col_values[3]);

			if (m_url_to_domain->has_url(target_url.hash())) {

				URL source_url(col_values[0], col_values[1]);

				float target_harmonic = domain_stats::harmonic_centrality(target_url);
				float source_harmonic = domain_stats::harmonic_centrality(source_url);

				const string link_text = col_values[4].substr(0, 1000);

				const Link::Link link(source_url, target_url, source_harmonic, target_harmonic);

				uint64_t link_hash = source_url.link_hash(target_url, link_text);

				vector<string> words = text::get_expanded_full_text_words(link_text);
				for (const string &word : words) {

					const uint64_t word_hash = Hash::str(word);

					link_record rec(link_hash, source_harmonic);
					rec.m_source_domain = source_url.host_hash();
					rec.m_target_hash = target_url.hash();
					m_link_index_builder->add(word_hash, rec);
				}
			}
			if (m_url_to_domain->has_domain(target_url.host_hash())) {
				URL source_url(col_values[0], col_values[1]);

				float target_harmonic = domain_stats::harmonic_centrality(target_url);
				float source_harmonic = domain_stats::harmonic_centrality(source_url);

				const string link_text = col_values[4].substr(0, 1000);

				const Link::Link link(source_url, target_url, source_harmonic, target_harmonic);

				uint64_t link_hash = source_url.domain_link_hash(target_url, link_text);

				vector<string> words = text::get_expanded_full_text_words(link_text);
				for (const string &word : words) {

					const uint64_t word_hash = Hash::str(word);

					domain_link_record rec(link_hash, source_harmonic);
					rec.m_source_domain = source_url.host_hash();
					rec.m_target_domain = target_url.host_hash();
					m_domain_link_index_builder->add(word_hash, rec);
				}
			}
		}
	}

	void index_tree::add_link_files_threaded(const vector<string> &local_paths, size_t num_threads) {

		m_url_to_domain->read();

		utils::thread_pool pool(num_threads);

		for (auto &local_path : local_paths) {
			pool.enqueue([this, local_path]() -> void {
				add_link_file(local_path);
			});
		}

		pool.run_all();
	}

	void index_tree::merge() {
		for (level *lvl : m_levels) {
			lvl->merge();
		}
		//m_hash_table->merge();

		m_link_index_builder->append();
		m_link_index_builder->merge();
		m_domain_link_index_builder->append();
		m_domain_link_index_builder->merge();
	}

	void index_tree::truncate() {
		for (level *lvl : m_levels) {
			delete_directories(lvl->get_type());
			create_directories(lvl->get_type());
		}

		m_link_index_builder->truncate();
		m_domain_link_index_builder->truncate();
	}

	void index_tree::clean_up() {
		for (level *lvl : m_levels) {
			lvl->clean_up();
		}
	}

	void index_tree::calculate_scores_for_level(size_t level_num) {
		m_levels[level_num]->calculate_scores();
	}

	std::vector<return_record> index_tree::find(const string &query) {

		vector<link_record> links = m_link_index->find(text::get_tokens(query));
		vector<domain_link_record> domain_links = m_domain_link_index->find(text::get_tokens(query));

		//for (auto &link : links) link.m_score = 0.2;
		//for (auto &link : domain_links) link.m_score = 0.2;

		std::vector<return_record> res = find_recursive(query, 0, {0}, links, domain_links);

		// Sort by score.
		std::sort(res.begin(), res.end(), [](const return_record &a, const return_record &b) {
			return a.m_score > b.m_score;
		});

		return res;
	}

	std::vector<return_record> index_tree::find_recursive(const string &query, size_t level_num,
		const std::vector<size_t> &keys, const vector<link_record> &links,
		const vector<domain_link_record> &domain_links) {

		std::vector<return_record> all_results = m_levels[level_num]->find(query, keys, links, domain_links);
		
		if (level_num == m_levels.size() - 1) {
			// This is the last level, return the results instead of going deeper.
			return all_results;
		}
		// Go deeper. The m_value of results are keys for the next level...
		std::vector<size_t> next_level_keys;
		for (const return_record &rec : all_results) {
			next_level_keys.push_back(rec.m_value);
		}
		return find_recursive(query, level_num + 1, next_level_keys, links, domain_links);
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
