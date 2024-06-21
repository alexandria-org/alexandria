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

#include "index_manager.h"
#include "merger.h"
#include "domain_stats/domain_stats.h"
#include "url_link/link.h"
#include "algorithm/algorithm.h"
#include "algorithm/sort.h"
#include "utils/thread_pool.hpp"

using namespace std;

namespace indexer {

	index_manager::index_manager() {

		m_url_index_builder = std::make_unique<sharded_builder<basic_index_builder, url_record>>("url_index", 4001);
		m_url_index = std::make_unique<sharded<basic_index, url_record>>("url_index", 4001);

		m_link_index_builder = std::make_unique<sharded_builder<basic_index_builder, link_record>>("link_index", 4001);
		m_link_index = std::make_unique<sharded<basic_index, link_record>>("link_index", 4001);

		m_domain_link_index_builder = std::make_unique<sharded_builder<basic_index_builder, domain_link_record>>("domain_link_index", 4001);
		m_domain_link_index = std::make_unique<sharded<basic_index, domain_link_record>>("domain_link_index", 4001);

		m_hash_table_builder = std::make_unique<hash_table2::builder>("index_manager");
		m_hash_table = std::make_unique<hash_table2::hash_table>("index_manager");

	}

	index_manager::~index_manager() {
	}

	void index_manager::add_index_file(const string &local_path) {

		const vector<size_t> cols = {1, 2, 3, 4};
		const vector<float> scores = {10.0, 3.0, 2.0, 1};

		ifstream infile(local_path, ios::in);
		string line;

		// word_map holds a word hash (token) => score
		std::map<uint64_t, float> word_map;

		size_t num_added = 0;
		while (getline(infile, line)) {
			vector<string> col_values;
			boost::algorithm::split(col_values, line, boost::is_any_of("\t"));


			URL url(col_values[0]);

			const uint64_t url_hash = url.hash();
			const uint64_t domain_hash = url.host_hash();
			const float harmonic = domain_stats::harmonic_centrality(url);

			// add to hash table
			m_hash_table_builder->add(url_hash, line);

			url_record record(url_hash, 0.0f, domain_hash);
			record.url_length(url.path_with_query().size());

			const std::string site_colon = "site:" + url.host() + " site:www." + url.host() + " " + url.host() + " " + url.domain_without_tld();
			const auto site_colon_tokens = text::get_unique_full_text_tokens(site_colon);
			for (auto token : site_colon_tokens) {
				word_map[token] += harmonic * 20;
			}

			size_t col_idx = 0;
			for (size_t col : cols) {
				const auto tokens = text::get_unique_expanded_full_text_tokens(col_values[col]);
				for (auto token : tokens) {
					word_map[token] += scores[col_idx] * harmonic;
				}
			}

			for (const auto &iter : word_map) {
				record.m_score = iter.second;
				m_url_index_builder->add(iter.first, record);
				num_added++;
			}

			word_map.clear();
		}
		std::cout << "num added: " << num_added << std::endl;
	}

	void index_manager::add_index_files_threaded(const vector<string> &local_paths, size_t num_threads) {

		num_threads = 1;
		utils::thread_pool pool(num_threads);

		for (const string &local_path : local_paths) {
			pool.enqueue([this, local_path]() -> void {
				add_index_file(local_path);
			});
		}

		pool.run_all();

		m_hash_table_builder->merge();

	}

	void index_manager::add_link_file(const string &local_path, const ::algorithm::bloom_filter &urls_to_index) {

		profiler::instance prof("add " + local_path);
		ifstream infile(local_path, ios::in);
		string line;
		size_t added = 0;
		size_t parsed = 0;
		std::vector<std::string> col_values;
		while (getline(infile, line)) {

			col_values.clear();
			boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

			URL target_url(col_values[2], col_values[3]);

			parsed++;

			URL source_url(col_values[0], col_values[1]);

			float target_harmonic = domain_stats::harmonic_centrality(target_url);
			float source_harmonic = domain_stats::harmonic_centrality(source_url);

			const std::string link_text = col_values[4].substr(0, 1000);

			const url_link::link link(source_url, target_url, source_harmonic, target_harmonic);

			const uint64_t domain_link_hash = source_url.domain_link_hash(target_url, link_text);
			const uint64_t link_hash = source_url.link_hash(target_url, link_text);
			const bool bloom_has_url = urls_to_index.exists(target_url.hash());

			std::vector<uint64_t> tokens = text::get_unique_expanded_full_text_tokens(link_text);

			if (bloom_has_url) {

				const bool has_url = m_hash_table->has(target_url.hash());

				if (has_url) {
					// Add the url link.
					link_record link_rec(link_hash, source_harmonic);
					link_rec.m_source_domain = source_url.hash();
					link_rec.m_target_hash = target_url.hash();

					for (auto token : tokens) {
						m_link_index_builder->add(token, link_rec);
					}
					added++;
				}
			}

			domain_link_record rec(domain_link_hash, source_harmonic);
			rec.m_source_domain = source_url.host_hash();
			rec.m_target_domain = target_url.host_hash();

			for (auto token : tokens) {
				m_domain_link_index_builder->add(token, rec);
			}

			tokens.clear();
		}

		cout << "Done with " << local_path << " added " << added << " total " << parsed << " took: " << prof.get() << "ms" << endl;
	}

	void index_manager::add_link_files_threaded(const vector<string> &local_paths, size_t num_threads, const ::algorithm::bloom_filter &urls_to_index) {

		utils::thread_pool pool(num_threads);

		for (auto &local_path : local_paths) {
			pool.enqueue([this, local_path, &urls_to_index]() -> void {
				add_link_file(local_path, urls_to_index);
			});
		}

		pool.run_all();
	}

	void index_manager::add_url_file(const string &local_path) {

		
	}

	void index_manager::add_url_files_threaded(const vector<string> &local_paths, size_t num_threads) {

		utils::thread_pool pool(num_threads);

		for (auto &local_path : local_paths) {
			pool.enqueue([this, local_path]() -> void {
				add_url_file(local_path);
			});
		}

		pool.run_all();
	}

	void index_manager::merge() {

		m_url_index_builder->append();
		m_url_index_builder->merge();

		m_link_index_builder->append();
		m_link_index_builder->merge();

		m_domain_link_index_builder->append();
		m_domain_link_index_builder->merge();

	}

	void index_manager::optimize() {
	}

	void index_manager::truncate() {
		m_url_index_builder->truncate();
		truncate_links();
	}

	void index_manager::truncate_links() {
		m_link_index_builder->truncate();
		m_domain_link_index_builder->truncate();
	}

	std::vector<return_record> index_manager::find(const string &query, full_text::search_metric &metric) {

		auto words = text::get_full_text_words(query, config::query_max_words);
		if (words.size() == 0) return {};

		auto tokens = text::get_full_text_tokens(query, config::query_max_words);

		auto links = m_link_index->find_intersection(tokens, 500000);

		metric.m_total_url_links_found = links.size();
		metric.m_links_handled = links.size();

		std::sort(links.begin(), links.end(), [](const auto &a, const auto &b) {
			return a.m_target_hash < b.m_target_hash;
		});

		auto domain_links = m_domain_link_index->find_intersection(tokens, 100000);

		metric.m_total_domain_links_found = domain_links.size();

		auto results = m_url_index->find_intersection(tokens);

		metric.m_total_found = results.size();

		size_t applied_domain_links = apply_domain_link_scores(domain_links, results);
		size_t applied_url_links = apply_link_scores(links, results);

		metric.m_link_url_matches = applied_url_links;
		metric.m_link_domain_matches = applied_domain_links;

		const auto sort_by = [](const auto &a, const auto &b) {
			if (a.m_score == b.m_score) return a.m_value < b.m_value;
			return a.m_score > b.m_score;
		};

		if (results.size() > config::pre_result_limit) {
			nth_element(results.begin(), results.begin() + (config::pre_result_limit - 1), results.end(), sort_by);
			std::sort(results.begin(), results.begin() + config::pre_result_limit, sort_by);
			results.resize(config::pre_result_limit);
		}

		const auto deduplicated = deduplicate_search_results(results, config::result_limit);
		const auto return_records = decorate_search_result(deduplicated);

		return return_records;
	}

	std::vector<url_record> index_manager::deduplicate_search_results(const std::vector<url_record> &results, size_t limit) {

		std::vector<url_record> deduped;
		std::vector<url_record> non_deduped;

		std::map<uint64_t, size_t> d_count;
		for (const auto &result : results) {
			if (d_count[result.m_domain_hash] < config::deduplicate_domain_count) {
				deduped.push_back(result);
			} else {
				non_deduped.push_back(result);
			}
			d_count[result.m_domain_hash]++;
		}
		if (deduped.size() < limit) {
			const size_t num_missing = limit - deduped.size();
			if (non_deduped.size() > num_missing) {
				non_deduped.resize(num_missing);
			}
			std::vector<url_record> ret;
			::algorithm::sort::merge_arrays(deduped, non_deduped, [] (const auto &a, const auto &b) {
				return a.m_score > b.m_score;
			}, ret);
			return ret;
		}

		deduped.resize(limit);

		return deduped;
	}

	std::vector<return_record> index_manager::decorate_search_result(const std::vector<url_record> &results) {
		std::vector<return_record> return_records;

		for (const auto &res : results) {
			const auto tsv_data = m_hash_table->find(res.m_value);
			return_record ret(res.m_value, res.m_score, tsv_data);
			ret.m_domain_hash = res.m_domain_hash;
			return_records.push_back(std::move(ret));
		}

		return return_records;
	}

	size_t index_manager::apply_domain_link_scores(const vector<domain_link_record> &links, std::vector<url_record> &results) {
		if (links.size() == 0) return 0;
		size_t applied_links = 0;

		{
			unordered_map<uint64_t, float> domain_scores;
			unordered_map<uint64_t, int> domain_counts;
			map<pair<uint64_t, uint64_t>, uint64_t> domain_unique;
			{
				for (const auto &link : links) {
					if (domain_unique.count(std::make_pair(link.m_source_domain, link.m_target_domain)) == 0) {
						const float domain_score = expm1(25.0f*link.m_score) / 50.0f;
						domain_scores[link.m_target_domain] += domain_score;
						domain_counts[link.m_target_domain]++;
						domain_unique[std::make_pair(link.m_source_domain, link.m_target_domain)] = link.m_source_domain;
					}
				}
			}

			for (auto &result : results) {
				const float domain_score = domain_scores[result.m_domain_hash];
				result.m_score += domain_score;
				applied_links += domain_counts[result.m_domain_hash];
			}
		}

		return applied_links;
	}

	size_t index_manager::apply_link_scores(const vector<link_record> &links, std::vector<url_record> &results) {

		if (links.size() == 0) return 0;

		size_t applied_links = 0;

		size_t i = 0, j = 0;
		std::map<std::pair<uint64_t, uint64_t>, uint64_t> domain_unique;
		while (i < links.size() && j < results.size()) {
			const uint64_t hash1 = links[i].m_target_hash;
			const uint64_t hash2 = results[j].m_value;

			if (hash1 < hash2) {
				i++;
			} else if (hash1 == hash2) {
				if (domain_unique.count(std::make_pair(links[i].m_source_domain, links[i].m_target_hash)) == 0) {
					const float url_score = expm1(25.0f*links[i].m_score) / 50.0f;
					results[j].m_score += url_score;
					applied_links++;
					domain_unique[std::make_pair(links[i].m_source_domain, links[i].m_target_hash)] = links[i].m_source_domain;
				}

				i++;
			} else {
				j++;
			}
		}
		return applied_links;
	}

	std::vector<return_record> index_manager::find(const string &query) {
		full_text::search_metric metric;
		return find(query, metric);
	}

}
