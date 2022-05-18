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
#include "utils/thread_pool.hpp"
#include "domain_level.h"
#include "url_level.h"

using namespace std;

namespace indexer {

	index_manager::index_manager() {

		m_link_index_builder = std::make_unique<sharded_builder<counted_index_builder, link_record>>("link_index", 4001);
		m_link_index = std::make_unique<sharded<counted_index, link_record>>("link_index", 4001);

		m_domain_link_index_builder = std::make_unique<sharded_builder<counted_index_builder, domain_link_record>>("domain_link_index", 4001);
		m_domain_link_index = std::make_unique<sharded<counted_index, domain_link_record>>("domain_link_index", 4001);

		m_hash_table = std::make_unique<hash_table::builder>("index_manager");
		m_hash_table_words = std::make_unique<hash_table::builder>("word_hash_table");
		m_url_to_domain = std::make_unique<full_text::url_to_domain>("index_manager");

		m_title_word_builder = std::make_unique<sharded_builder<counted_index_builder, counted_record>>("title_word_counter", 997);
		m_title_word_counter = std::make_unique<sharded<counted_index, counted_record>>("title_word_counter", 997);

		m_link_word_builder = std::make_unique<sharded_builder<counted_index_builder, counted_record>>("link_word_counter", 4001);
		m_link_word_counter = std::make_unique<sharded<counted_index, counted_record>>("link_word_counter", 4001);

		//m_word_index_builder = std::make_unique<sharded_builder<counted_index_builder, counted_record>>("word_index", 256);
		m_word_index = std::make_unique<sharded<counted_index, counted_record>>("word_index", 256);
	}

	index_manager::~index_manager() {
	}

	void index_manager::add_level(level *lvl) {
		create_directories(lvl->get_type());
		m_levels.push_back(lvl);
	}

	void index_manager::add_snippet(const snippet &s) {
		for (level *lvl : m_levels) {
			lvl->add_snippet(s);
		}
	}

	void index_manager::add_document(size_t id, const string &doc) {
		for (level *lvl : m_levels) {
			lvl->add_document(id, doc);
		}
	}

	void index_manager::add_index_file(const string &local_path) {
		for (level *lvl : m_levels) {
			lvl->add_index_file(local_path, [this](uint64_t key, const string &value) {
				m_hash_table->add(key, value);
			}, [this](uint64_t url_hash, uint64_t domain_hash) {
				m_url_to_domain->add_url(url_hash, domain_hash);
			});
		}
	}

	void index_manager::add_index_files_threaded(const vector<string> &local_paths, size_t num_threads) {

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

	void index_manager::add_link_file(const string &local_path, const ::algorithm::bloom_filter &urls_to_index) {

		profiler::instance prof("add " + local_path);
		ifstream infile(local_path, ios::in);
		string line;
		size_t added = 0;
		size_t parsed = 0;
		vector<string> col_values;
		set<uint64_t> tokens;
		while (getline(infile, line)) {

			col_values.clear();
			boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

			URL target_url(col_values[2], col_values[3]);

			parsed++;

			// Check if we have the url
			//if (!urls_to_index.exists(target_url.hash_input())) continue;

			added++;

			URL source_url(col_values[0], col_values[1]);

			float target_harmonic = domain_stats::harmonic_centrality(target_url);
			float source_harmonic = domain_stats::harmonic_centrality(source_url);

			const string link_text = col_values[4].substr(0, 1000);

			const url_link::link link(source_url, target_url, source_harmonic, target_harmonic);

			const uint64_t domain_link_hash = source_url.domain_link_hash(target_url, link_text);
			const uint64_t link_hash = source_url.link_hash(target_url, link_text);
			const bool has_url = true; //urls_to_index.count(target_url.hash());

			vector<string> words = text::get_expanded_full_text_words(link_text);

			text::words_to_ngram_hash(words, 3, [&tokens](const uint64_t hash) {
				tokens.insert(hash);
			});

			if (has_url) {
				// Add the url link.
				link_record link_rec(link_hash, source_harmonic);
				link_rec.m_source_domain = source_url.hash();
				link_rec.m_target_hash = target_url.hash();

				for (auto token : tokens) {
					m_link_index_builder->add(token, link_rec);
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

	void index_manager::add_title_file(const string &local_path) {
		const size_t title_col = 1;

		ifstream infile(local_path, ios::in);
		string line;
		std::map<uint64_t, std::string> word_index;
		set<uint64_t> tokens;
		while (getline(infile, line)) {
			vector<string> col_values;
			boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

			URL url(col_values[0]);

			uint64_t domain_hash = url.host_hash();

			vector<string> words = text::get_full_text_words(col_values[title_col]);

			text::words_to_ngram_hash(words, 3, [&tokens](const uint64_t hash) {
				tokens.insert(hash);
			});
			text::words_to_ngram_hash(words, 3, [&tokens, &word_index](const uint64_t hash, const string &ngram) {
				tokens.insert(hash);

				if (word_index.count(hash) == 0) {
					word_index[hash] = ngram;
				}
			});
			for (auto word_hash : tokens) {
				m_title_word_builder->add(domain_hash, counted_record(word_hash, 0.0f));
			}
			tokens.clear();
		}

		for (const auto &iter : word_index) {
			m_hash_table_words->add(iter.first, iter.second);
		}
	}

	void index_manager::add_title_files_threaded(const vector<string> &local_paths, size_t num_threads) {

		utils::thread_pool pool(num_threads);

		for (auto &local_path : local_paths) {
			pool.enqueue([this, local_path]() -> void {
				add_title_file(local_path);
			});
		}

		pool.run_all();
	}

	void index_manager::add_link_count_file(const string &local_path) {

		ifstream infile(local_path, ios::in);
		string line;
		std::map<uint64_t, std::string> word_index;
		set<uint64_t> tokens;
		while (getline(infile, line)) {
			vector<string> col_values;
			boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

			URL target_url(col_values[2], col_values[3]);
			const uint64_t domain_hash = target_url.host_hash();

			const string link_text = col_values[4].substr(0, 1000);

			vector<string> words = text::get_full_text_words(link_text);

			text::words_to_ngram_hash(words, 3, [&tokens, &word_index](const uint64_t hash, const string &ngram) {
				tokens.insert(hash);

				if (word_index.count(hash) == 0) {
					word_index[hash] = ngram;
				}
			});
			for (auto word_hash : tokens) {
				m_link_word_builder->add(domain_hash, counted_record(word_hash, 0.0f));
			}
			tokens.clear();
		}

		for (const auto &iter : word_index) {
			m_hash_table_words->add(iter.first, iter.second);
		}
	}

	void index_manager::add_link_count_files_threaded(const vector<string> &local_paths, size_t num_threads) {

		utils::thread_pool pool(num_threads);

		for (auto &local_path : local_paths) {
			pool.enqueue([this, local_path]() -> void {
				add_link_count_file(local_path);
			});
		}

		pool.run_all();
	}

	/*
	 * Add each url to its own domains index.
	 */
	void index_manager::add_url_file(const string &local_path) {

		for (level *lvl : m_levels) {
			lvl->add_index_file(local_path, [this](uint64_t key, const string &value) {
			}, [this](uint64_t url_hash, uint64_t domain_hash) {
			});
		}
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

	void index_manager::add_word_file(const string &local_path, const std::set<uint64_t> &words_to_index) {

		const vector<size_t> cols = {1, 2, 3, 4};
		const vector<size_t> field_multipliers = {10, 3, 2, 1};

		map<uint64_t, size_t> counts;

		ifstream infile(local_path, ios::in);
		string line;
		while (getline(infile, line)) {

			vector<string> col_values;
			boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

			URL url(col_values[0]);
			const uint64_t domain_hash = url.host_hash();

			for (size_t ii = 0; ii < cols.size(); ii++) {
				size_t col = cols[ii];
				vector<string> words = text::get_full_text_words(col_values[col]);

				for (const string &word : words) {
					const uint64_t word_hash = ::algorithm::hash(word);
					if (words_to_index.find(word_hash) != words_to_index.end()) {
						counts[word_hash] = field_multipliers[ii];
					}
				}

				for (const auto &iter : counts) {
					m_word_index_builder->add(iter.first, counted_record(domain_hash, 0.0f, iter.second));
				}
				counts.clear();
			}

		}
	}

	void index_manager::add_word_files_threaded(const vector<string> &local_paths, size_t num_threads, const std::set<uint64_t> &words_to_index) {


		LOG_INFO("done... found " + to_string(words_to_index.size()) + " words");
		LOG_INFO("running add_word_files on " + to_string(local_paths.size()) + " files");

		utils::thread_pool pool(num_threads);

		for (const string &local_path : local_paths) {
			pool.enqueue([this, local_path, &words_to_index]() -> void {
				add_word_file(local_path, words_to_index);
			});
		}

		pool.run_all();

	}

	void index_manager::merge() {
		for (level *lvl : m_levels) {
			lvl->merge();
		}
		//m_hash_table->merge();

		m_link_index_builder->append();
		m_link_index_builder->merge();
		m_domain_link_index_builder->append();
		m_domain_link_index_builder->merge();

	}

	void index_manager::optimize() {
	}

	void index_manager::merge_word() {
		m_word_index_builder->append();
		m_word_index_builder->merge();
	}

	void index_manager::truncate() {
		for (level *lvl : m_levels) {
			delete_directories(lvl->get_type());
			create_directories(lvl->get_type());
		}

		truncate_links();
	}

	void index_manager::truncate_links() {
		m_link_index_builder->truncate();
		m_domain_link_index_builder->truncate();
	}

	void index_manager::truncate_words() {
		m_word_index_builder->truncate();
	}

	void index_manager::clean_up() {
		for (level *lvl : m_levels) {
			lvl->clean_up();
		}
	}

	void index_manager::calculate_scores_for_level(size_t level_num) {
		m_levels[level_num]->calculate_scores();
	}

	std::vector<return_record> index_manager::find(const string &query) {

		auto domain_formula = [](float score) {
			return expm1(25.0f * score) / 50.0f;
		};

		std::vector<size_t> counts;

		vector<counted_record> bm25_scores = m_word_index->find_sum(text::get_tokens(query), 100000);
		vector<link_record> links = m_link_index->find_intersection(text::get_tokens(query));
		vector<domain_link_record> domain_links = m_domain_link_index->find_intersection(text::get_tokens(query));
		cout << "found: " << links.size() << " links" << endl;
		cout << "found: " << domain_links.size() << " domain links" << endl;

		std::sort(links.begin(), links.end(), link_record::storage_order());
		std::sort(domain_links.begin(), domain_links.end(), domain_link_record::storage_order());

		// group by target domain.
		std::vector<domain_link_record> grouped;
		for (auto rec : domain_links) {
			if (grouped.size() && grouped.back().storage_equal(rec)) {
				grouped.back().m_score += domain_formula(rec.m_score);
			} else {
				grouped.emplace_back(rec);
				grouped.back().m_score = domain_formula(rec.m_score);
			}
		}


		if (!std::is_sorted(bm25_scores.begin(), bm25_scores.end())) {
			throw new runtime_error("bm25 are not sorted");
		}
		if (!std::is_sorted(links.begin(), links.end(), link_record::storage_order())) {
			throw new runtime_error("links are not sorted");
		}
		if (!std::is_sorted(grouped.begin(), grouped.end(), domain_link_record::storage_order())) {
			throw new runtime_error("doain_links are not sorted");
		}

		profiler::instance inst("m_levels[0]->find");
		std::vector<return_record> res = m_levels[0]->find(query, {}, links, grouped, bm25_scores);
		inst.stop();

		// Sort by score.
		std::sort(res.begin(), res.end(), [](const return_record &a, const return_record &b) {
			return a.m_score > b.m_score;
		});

		return res;
	}

	void index_manager::create_directories(level_type lvl) {
		for (size_t i = 0; i < 8; i++) {
			boost::filesystem::create_directories("/mnt/" + std::to_string(i) + "/full_text/" + level_to_str(lvl));
		}
	}

	void index_manager::delete_directories(level_type lvl) {
		for (size_t i = 0; i < 8; i++) {
			boost::filesystem::remove_all("/mnt/" + std::to_string(i) + "/full_text/" + level_to_str(lvl));
		}
	}
}
