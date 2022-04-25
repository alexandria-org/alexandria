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

#include <iostream>
#include <vector>
#include <cmath>
#include "full_text/full_text_index.h"
#include "full_text/full_text_record.h"
#include "full_text/full_text_shard.h"
#include "full_text/search_metric.h"
#include "url_link/full_text_record.h"
#include "domain_link/full_text_record.h"
#include "logger/logger.h"
#include "profiler/profiler.h"
#include "parser/parser.h"
#include "transfer/transfer.h"
#include "algorithm/hash.h"
#include "algorithm/algorithm.h"
#include "algorithm/sort.h"
#include "search_allocation/search_allocation.h"
#include <cassert>

namespace search_engine {

	using std::string;
	using std::vector;
	using std::future;
	using std::thread;
	using std::span;
	using std::pair;
	using std::map;
	using std::unordered_map;
	using full_text::full_text_index;
	using full_text::full_text_result_set;
	using full_text::search_metric;
	using full_text::full_text_shard;

	/*
		Public interface
	*/

	/*
		Our main search routine, no deduplication just raw search.
	*/
	template<typename data_record>
	vector<data_record> search(search_allocation::storage<data_record> *storage, const full_text_index<data_record> &index,
		const vector<url_link::full_text_record> &links, const vector<domain_link::full_text_record> &domain_links, const string &query, size_t limit,
		struct search_metric &metric);

	/*
		Only for full_text_records since deduplication requires domain hashes.
	*/
	vector<full_text::full_text_record> search_deduplicate(search_allocation::storage<full_text::full_text_record> *storage,
		const full_text_index<full_text::full_text_record> &index, const vector<url_link::full_text_record> &links,
		const vector<domain_link::full_text_record> &domain_links, const string &query, size_t limit, struct search_metric &metric);

	/*
		Search for the exact phrase. Will treat the whole phrase as an n_gram so will only give results when num words in query are less
		or equal to config::n_gram.
	*/
	template<typename data_record>
	vector<data_record> search_exact(search_allocation::storage<data_record> *storage, const full_text_index<data_record> &index,
		const string &query, size_t limit, struct search_metric &metric);

	template<typename data_record>
	vector<data_record> search_ids(search_allocation::storage<data_record> *storage, const full_text_index<data_record> &index,
		const string &query, size_t limit);

	template<typename data_record>
	full_text_result_set<data_record> *search_remote(const std::string &query, search_allocation::storage<data_record> *storage);

}

namespace search_engine {

	template<typename data_record>
	class comparator_class {
	public:
		// Comparator function
		bool operator()(data_record &a, data_record &b)
		{
			if (a.m_score == b.m_score) return a.m_value < b.m_value;
			return a.m_score > b.m_score;
		}
	};

	void reset_search_metric(struct search_metric &metric);

	template<typename data_record>
	void set_total_found(const vector<full_text_result_set<data_record> *> result_vector, struct search_metric &metric, double result_quote) {

		size_t largest_total = 0;
		for (full_text_result_set<data_record> *result : result_vector) {
			if (result->total_num_results() > largest_total) {
				largest_total = result->total_num_results();
			}
		}

		metric.m_total_found = (size_t)(largest_total * result_quote);
	}

	template<typename data_record>
	size_t largest_result(const vector<full_text_result_set<data_record> *> &result_vector) {

		size_t largest_size = 0;
		for (full_text_result_set<data_record> *result : result_vector) {
			if (result->size() > largest_size) {
				largest_size = result->size();
			}
		}

		return largest_size;
	}

	/*
		Add scores for the given links to the result set. The links are assumed to be ordered by link.m_target_hash ascending.
	*/
	template<typename data_record>
	size_t apply_link_scores(const vector<url_link::full_text_record> &links, full_text_result_set<data_record> *results) {

		if (typeid(data_record) != typeid(full_text::full_text_record)) return 0;
		if (links.size() == 0) return 0;

		size_t applied_links = 0;

		size_t i = 0;
		size_t j = 0;
		map<pair<uint64_t, uint64_t>, uint64_t> domain_unique;
		full_text::full_text_record *data = (full_text::full_text_record *)results->data_pointer();
		while (i < links.size() && j < results->size()) {

			const uint64_t hash1 = links[i].m_target_hash;
			const uint64_t hash2 = data[j].m_value;

			if (hash1 < hash2) {
				i++;
			} else if (hash1 == hash2) {

				if (domain_unique.count(std::make_pair(links[i].m_source_domain, links[i].m_target_hash)) == 0) {
					const float url_score = expm1(25.0f*links[i].m_score) / 50.0f;
					data[j].m_score += url_score;
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

	template<typename data_record>
	size_t apply_domain_link_scores(const vector<domain_link::full_text_record> &links, full_text_result_set<data_record> *results) {

		if (typeid(data_record) != typeid(full_text::full_text_record)) return 0;
		if (links.size() == 0) return 0;

		size_t applied_links = 0;
		{
			unordered_map<uint64_t, float> domain_scores;
			unordered_map<uint64_t, int> domain_counts;
			map<pair<uint64_t, uint64_t>, uint64_t> domain_unique;
			{
				for (const domain_link::full_text_record &link : links) {

					if (domain_unique.count(std::make_pair(link.m_source_domain, link.m_target_domain)) == 0) {

						const float domain_score = expm1(25.0f*link.m_score) / 50.0f;
						domain_scores[link.m_target_domain] += domain_score;
						domain_counts[link.m_target_domain]++;
						domain_unique[std::make_pair(link.m_source_domain, link.m_target_domain)] = link.m_source_domain;

					}
				}
			}

			// Loop over the results and add the calculated domain scores.
			full_text::full_text_record *data = (full_text::full_text_record *)results->data_pointer();
			for (size_t i = 0; i < results->size(); i++) {
				const float domain_score = domain_scores[data[i].m_domain_hash];
				data[i].m_score += domain_score;
				applied_links += domain_counts[data[i].m_domain_hash];
			}
		}

		return applied_links;
	}

	template<typename data_record>
	size_t lower_bound(const data_record *data, size_t pos, size_t len, uint64_t value) {
		while (pos < len) {
			size_t m = (pos + len) >> 1;
			if (data[m].m_value < value) {
				pos = m + 1;
			} else {
				len = m;
			}
		}

		return pos;
	}

	template<typename data_record>
	void value_intersection(const vector<full_text_result_set<data_record> *> &result_sets, vector<int> sections, vector<data_record> &dest) {

		if (result_sets.size() == 0) {
			return;
		}

		size_t shortest_vector_position = 0;
		size_t shortest_len = SIZE_MAX;
		{
			size_t iter_index = 0;
			for (full_text_result_set<data_record> *result_set : result_sets) {
				if (shortest_len > result_set->size()) {
					shortest_len = result_set->size();
					shortest_vector_position = iter_index;
				}
				iter_index++;
			}
		}

		vector<size_t> positions(result_sets.size(), 0);

		const data_record *shortest_data = result_sets[shortest_vector_position]->section_pointer(sections[shortest_vector_position]);

		while (positions[shortest_vector_position] < shortest_len) {

			bool all_equal = true;
			uint64_t value = shortest_data[positions[shortest_vector_position]].m_value;

			float score_sum = 0.0f;
			size_t iter_index = 0;
			for (full_text_result_set<data_record> *result_set : result_sets) {
				const data_record *data_arr = result_set->section_pointer(sections[iter_index]);
				const size_t len = result_set->size();

				size_t *pos = &(positions[iter_index]);
				
				// this is a linear search.
				while (*pos < len && value > data_arr[*pos].m_value) {
					(*pos)++;
				}

				if (*pos < len && value == data_arr[*pos].m_value) {
					const float score = data_arr[*pos].m_score;
					score_sum += score;
				}
				if ((*pos < len && value < data_arr[*pos].m_value) || *pos >= len) {
					all_equal = false;
					break;
				}
				iter_index++;
			}
			if (all_equal) {
				dest.push_back(shortest_data[positions[shortest_vector_position]]);
				dest.back().m_score = score_sum / result_sets.size();
			}

			positions[shortest_vector_position]++;
		}
	}

	template<typename data_record>
	void calculate_intersection(const vector<full_text_result_set<data_record> *> &result_sets, full_text_result_set<data_record> *dest) {

		for (full_text_result_set<data_record> *result : result_sets) {
			if (result->size() == 0) return;
		}

		vector<full_text_result_set<data_record> *> sorted_result_sets(result_sets);

		sort(sorted_result_sets.begin(), sorted_result_sets.end(), [](const full_text_result_set<data_record> *a, const full_text_result_set<data_record> *b) {
			return a->total_num_results() < b->total_num_results();
		});

		vector<int> lengths;
		for (full_text_result_set<data_record> *result : sorted_result_sets) {
			lengths.push_back(result->num_sections());
		}

		vector<vector<int>> partitions = algorithm::incremental_partitions(lengths, config::ft_section_depth);

		// First just try the top sections.
		{
			vector<data_record> result;
			value_intersection(sorted_result_sets, partitions[0], result);
			if (result.size() >= config::result_limit) {
				dest->copy_vector(result);
				return;
			}
		}

		vector<int> maximum(sorted_result_sets.size(), 0);
		for (const vector<int> &vec : partitions) {
			for (size_t i = 0; i < vec.size(); i++) {
				if (vec[i] > maximum[i]) maximum[i] = vec[i];
			}
		}
		for (size_t i = 0; i < maximum.size(); i++) {
			sorted_result_sets[i]->read_to_section(maximum[i]);
		}

		size_t idx = 0;
		const size_t num_threads = 8;

		ThreadPool pool(num_threads);
		vector<vector<data_record>> results(partitions.size());
		std::vector<std::future<vector<data_record>>> thread_results;
		for (const vector<int> &partition : partitions) {
			thread_results.emplace_back(pool.enqueue([sorted_result_sets, partition]() {
				vector<data_record> result;
				value_intersection(sorted_result_sets, partition, result);
				return result;
			}));
			idx++;
		}
		idx = 0;
		for (auto && result: thread_results) {
			results[idx] = result.get();
			idx++;
		}
		// merge
		vector<data_record> merged_vec;
		algorithm::sort::merge_arrays(results, [](const data_record &a, const data_record &b) {
			return a.m_value < b.m_value;
		}, merged_vec);

		// copy.
		dest->copy_vector(merged_vec);
	}

	template<typename data_record>
	void sort_by_score(vector<data_record> &results) {
		sort(results.begin(), results.end(), [](const data_record &a, const data_record &b) {
			return a.m_score > b.m_score;
		});
	}

	/*
		puts the top n elements in the first n slots of results. Then sorts those top n elements by value.

		this function assumes that the input results are sorted by value! so it does nothing for n < results.size()
	*/
	template<typename data_record>
	void get_unsorted_results_with_top_scores(full_text_result_set<data_record> *result, size_t n) {

		if (result->size() > n) {
			span<data_record> *arr = result->span_pointer();
			nth_element(arr->begin(), arr->begin() + (n - 1), arr->end(), search_engine::comparator_class<data_record>{});

			sort(arr->begin(), arr->begin() + n, [](const data_record &a, const data_record &b) {
				return a.m_value < b.m_value;
			});

			result->resize(n);
		}
	}

	template<typename data_record>
	bool result_has_many_domains(const full_text_result_set<data_record> *results) {

		if (results->size() == 0) return false;

		const data_record *data = results->data_pointer();
		const uint64_t first_domain_hash = data[0].m_domain_hash;
		for (size_t i = 0; i < results->size(); i++) {
			if (data[i].m_domain_hash != first_domain_hash) {
				return true;
			}
		}

		return false;
	}

	template<typename data_record>
	void deduplicate_domains(full_text_result_set<data_record> *results, size_t results_per_domain, size_t limit) {

		vector<data_record> deduplicate;
		unordered_map<uint64_t, size_t> domain_counts;
		data_record *records = results->data_pointer();
		size_t j = 0;
		for (size_t i = 0; i < results->size() && j < limit; i++) {
			records[j] = records[i];
			if (domain_counts[records[i].m_domain_hash] < results_per_domain) {
				j++;
				domain_counts[records[i].m_domain_hash]++;
			}
		}
		results->resize(j);
	}

	template<typename data_record>
	vector<data_record> deduplicate_result_vector(const vector<data_record> &results, size_t limit) {

		vector<data_record> deduped;
		vector<data_record> non_deduped;

		map<uint64_t, size_t> d_count;
		for (const data_record &result : results) {
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
			vector<data_record> ret;
			algorithm::sort::merge_arrays(deduped, non_deduped, [] (const data_record &a, const data_record &b) {
				return a.m_score > b.m_score;
			}, ret);
			return ret;
		}

		deduped.resize(limit);

		return deduped;
	}

	template<typename data_record>
	vector<full_text_result_set<data_record> *> search_shards(vector<full_text_result_set<data_record> *> &result_sets,
		const vector<full_text_shard<data_record> *> &shards, const vector<string> &words) {

		assert(words.size() <= config::query_max_words);
		assert(words.size() <= result_sets.size());

		vector<full_text_result_set<data_record> *> result_vector;
		vector<string> searched_words;
		size_t word_id = 0;
		for (const string &word : words) {

			// One word should only be searched once.
			if (find(searched_words.begin(), searched_words.end(), word) != searched_words.end()) continue;
			
			searched_words.push_back(word);

			uint64_t word_hash = algorithm::hash(word);

			shards[word_hash % config::ft_num_shards]->find(word_hash, result_sets[word_id]);

			result_vector.push_back(result_sets[word_id]);
			word_id++;
		}

		return result_vector;
	}

	template<typename data_record>
	vector<full_text_result_set<data_record> *> search_shards_exact(vector<full_text_result_set<data_record> *> &result_sets,
		const vector<full_text_shard<data_record> *> &shards, const vector<string> &words) {

		assert(words.size() <= config::query_max_words);
		assert(words.size() <= result_sets.size());

		vector<full_text_result_set<data_record> *> result_vector;

		uint64_t n_gram_hash = algorithm::hash(boost::join(words, " "));

		shards[n_gram_hash % config::ft_num_shards]->find(n_gram_hash, result_sets[0]);

		result_vector.push_back(result_sets[0]);

		return result_vector;
	}

	template <typename data_record>
	full_text_result_set<data_record> *make_search(search_allocation::storage<data_record> *storage,
			const vector<full_text_shard<data_record> *> &shards, const vector<url_link::full_text_record> &links,
			const vector<domain_link::full_text_record> &domain_links, const string &query, size_t limit, struct search_metric &metric) {

		reset_search_metric(metric);

		vector<string> words = text::get_full_text_words(query, config::query_max_words);
		if (words.size() == 0) return new full_text_result_set<data_record>(0);

		vector<full_text_result_set<data_record> *> result_vector = search_shards<data_record>(storage->result_sets, shards, words);

		full_text_result_set<data_record> *flat_result;
		if (result_vector.size() > 1) {

			// We need to calculate the intersection of the given results.
			flat_result = storage->intersected_result;
			flat_result->resize(0);
			calculate_intersection<data_record>(result_vector, flat_result);

			set_total_found<data_record>(result_vector, metric, (double)flat_result->size() / largest_result(result_vector));
		} else {
			flat_result = result_vector[0];
			set_total_found<data_record>(result_vector, metric, 1.0);
		}

		// Close file pointers.
		for (full_text_result_set<data_record> *result_set : result_vector) {
			result_set->close_sections();
		}

		metric.m_link_domain_matches = apply_domain_link_scores(domain_links, flat_result);
		metric.m_link_url_matches = apply_link_scores(links, flat_result);

		get_unsorted_results_with_top_scores<data_record>(flat_result, limit);

		return flat_result;
	}

	template <typename data_record>
	full_text_result_set<data_record> *make_search_exact(search_allocation::storage<data_record> *storage,
			const vector<full_text_shard<data_record> *> &shards, const string &query, size_t limit, struct search_metric &metric) {

		reset_search_metric(metric);

		vector<string> words = text::get_full_text_words(query, config::query_max_words);
		if (words.size() == 0) return new full_text_result_set<data_record>(0);

		vector<full_text_result_set<data_record> *> result_vector = search_shards_exact<data_record>(storage->result_sets, shards, words);

		full_text_result_set<data_record> *flat_result;
		if (result_vector.size() > 1) {

			// We need to calculate the intersection of the given results.
			flat_result = storage->intersected_result;
			flat_result->resize(0);
			calculate_intersection<data_record>(result_vector, flat_result);

			set_total_found<data_record>(result_vector, metric, (double)flat_result->size() / largest_result(result_vector));
		} else {
			flat_result = result_vector[0];
			set_total_found<data_record>(result_vector, metric, 1.0);
		}

		// Close file pointers.
		for (full_text_result_set<data_record> *result_set : result_vector) {
			result_set->close_sections();
		}

		get_unsorted_results_with_top_scores<data_record>(flat_result, limit);

		return flat_result;
	}

	template<typename data_record>
	vector<data_record> search_wrapper(search_allocation::storage<data_record> *storage, const full_text_index<data_record> &index,
		const vector<url_link::full_text_record> &links, const vector<domain_link::full_text_record> &domain_links, const string &query, size_t limit,
		struct search_metric &metric) {


		full_text_result_set<data_record> *result = make_search<data_record>(storage, index.shards(), links, domain_links, query, limit, metric);

		vector<data_record> complete_result(result->span_pointer()->begin(), result->span_pointer()->end());

		// Sort.
		sort_by_score<data_record>(complete_result);

		return complete_result;
	}

	template<typename data_record>
	vector<data_record> search_wrapper_exact(search_allocation::storage<data_record> *storage, const full_text_index<data_record> &index,
		const string &query, size_t limit, struct search_metric &metric) {

		full_text_result_set<data_record> *result = make_search_exact<data_record>(storage, index.shards(), query, limit, metric);

		vector<data_record> complete_result(result->span_pointer()->begin(), result->span_pointer()->end());

		// Sort.
		sort_by_score<data_record>(complete_result);

		return complete_result;
	}

	template<typename data_record>
	vector<data_record> search(search_allocation::storage<data_record> *storage, const full_text_index<data_record> &index,
		const vector<url_link::full_text_record> &links, const vector<domain_link::full_text_record> &domain_links, const string &query, size_t limit,
		struct search_metric &metric) {

		vector<data_record> complete_result = search_wrapper(storage, index, links, domain_links, query, limit, metric);
		
		if (complete_result.size() > limit) {
			complete_result.resize(limit);
		}

		return complete_result;
	}

	template<typename data_record>
	vector<data_record> search_exact(search_allocation::storage<data_record> *storage, const full_text_index<data_record> &index,
		const string &query, size_t limit, struct search_metric &metric) {

		vector<data_record> complete_result = search_wrapper_exact(storage, index, query, limit, metric);
		
		if (complete_result.size() > limit) {
			complete_result.resize(limit);
		}

		return complete_result;
	}

	template<typename data_record>
	vector<data_record> search_ids(search_allocation::storage<data_record> *storage, const full_text_index<data_record> &index,
		const string &query, size_t limit) {

		(void)limit;

		vector<string> words = text::get_expanded_full_text_words(query);

		uint64_t key = algorithm::hash(boost::algorithm::join(words, " "));

		index.shards()[key % config::ft_num_shards]->find(key, storage->result_sets[0]);

		vector<data_record> ret(storage->result_sets[0]->span_pointer()->begin(), storage->result_sets[0]->span_pointer()->end());

		storage->result_sets[0]->close_sections();

		return ret;
	}

	template<typename data_record>
	full_text_result_set<data_record> *search_remote(const std::string &query, search_allocation::storage<data_record> *storage) {
		storage->result_sets[0]->resize(0);

		string buffer;
		int error;
		transfer::url_to_string(config::data_node + "/?i=" + parser::urlencode(query), buffer, error);
		if (error == transfer::OK) {
			const size_t num_records = buffer.size() / sizeof(data_record);
			data_record *data_ptr = storage->result_sets[0]->data_pointer();
			memcpy(data_ptr, buffer.c_str(), buffer.size());
			storage->result_sets[0]->resize(num_records);
		}
		return storage->result_sets[0];
	}

}
