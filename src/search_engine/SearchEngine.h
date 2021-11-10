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
#include "full_text/FullTextIndex.h"
#include "full_text/FullTextRecord.h"
#include "full_text/FullTextShard.h"
#include "full_text/SearchMetric.h"
#include "link_index/LinkFullTextRecord.h"
#include "link_index/DomainLinkFullTextRecord.h"
#include "system/Logger.h"
#include "system/Profiler.h"
#include "hash/Hash.h"
#include "sort/Sort.h"
#include "algorithm/Algorithm.h"
#include "SearchAllocation.h"
#include <cassert>

using namespace std;

namespace SearchEngine {
	/*
		Public interface
	*/

	/*
		Our main search routine, no deduplication just raw search.
	*/
	template<typename DataRecord>
	vector<DataRecord> search(SearchAllocation::Storage<DataRecord> *storage, vector<FullTextIndex<DataRecord> *> index_array,
		const vector<LinkFullTextRecord> &links, const vector<DomainLinkFullTextRecord> &domain_links, const string &query, size_t limit,
		struct SearchMetric &metric);

	/*
		Only for FullTextRecords since deduplication requires domain hashes.
	*/
	vector<FullTextRecord> search_deduplicate(SearchAllocation::Storage<FullTextRecord> *storage, vector<FullTextIndex<FullTextRecord> *> index_array,
		const vector<LinkFullTextRecord> &links, const vector<DomainLinkFullTextRecord> &domain_links, const string &query, size_t limit,
		struct SearchMetric &metric);

}

namespace SearchEngine {

	template<typename DataRecord>
	class comparator_class {
	public:
		// Comparator function
		bool operator()(DataRecord &a, DataRecord &b)
		{
			if (a.m_score == b.m_score) return a.m_value < b.m_value;
			return a.m_score > b.m_score;
		}
	};

	template<typename DataRecord>
	struct SearchArguments {
		string query;
		size_t limit;
		const vector<FullTextShard<DataRecord> *> *shards;
		struct SearchMetric *metric;
		const vector<LinkFullTextRecord> *links;
		const vector<DomainLinkFullTextRecord> *domain_links;
		SearchAllocation::Storage<DataRecord> *storage;
		size_t partition_id;
	};

	void reset_search_metric(struct SearchMetric &metric);

	template<typename DataRecord>
	void set_total_found(const vector<FullTextResultSet<DataRecord> *> result_vector, struct SearchMetric &metric, double result_quote) {

		size_t largest_total = 0;
		for (FullTextResultSet<DataRecord> *result : result_vector) {
			if (result->total_num_results() > largest_total) {
				largest_total = result->total_num_results();
			}
		}

		metric.m_total_found = (size_t)(largest_total * result_quote);
	}

	template<typename DataRecord>
	size_t largest_result(const vector<FullTextResultSet<DataRecord> *> result_vector) {

		size_t largest_size = 0;
		for (FullTextResultSet<DataRecord> *result : result_vector) {
			if (result->size() > largest_size) {
				largest_size = result->size();
			}
		}

		return largest_size;
	}

	/*
		Add scores for the given links to the result set. The links are assumed to be ordered by link.m_target_hash ascending.
	*/
	template<typename DataRecord>
	size_t apply_link_scores(const vector<LinkFullTextRecord> &links, FullTextResultSet<DataRecord> *results) {

		if (typeid(DataRecord) != typeid(FullTextRecord)) return 0;
		if (links.size() == 0) return 0;

		size_t applied_links = 0;

		size_t i = 0;
		size_t j = 0;
		map<pair<uint64_t, uint64_t>, uint64_t> domain_unique;
		FullTextRecord *data = (FullTextRecord *)results->data_pointer();
		while (i < links.size() && j < results->size()) {

			const uint64_t hash1 = links[i].m_target_hash;
			const uint64_t hash2 = data[j].m_value;

			if (hash1 < hash2) {
				i++;
			} else if (hash1 == hash2) {

				if (domain_unique.count(make_pair(links[i].m_source_domain, links[i].m_target_hash)) == 0) {
					const float url_score = expm1(25.0f*links[i].m_score) / 50.0f;
					data[j].m_score += url_score;
					applied_links++;
					domain_unique[make_pair(links[i].m_source_domain, links[i].m_target_hash)] = links[i].m_source_domain;
				}

				i++;
			} else {
				j++;
			}
		}

		return applied_links;
	}

	template<typename DataRecord>
	size_t apply_domain_link_scores(const vector<DomainLinkFullTextRecord> &links, FullTextResultSet<DataRecord> *results) {

		if (typeid(DataRecord) != typeid(FullTextRecord)) return 0;
		if (links.size() == 0) return 0;

		size_t applied_links = 0;
		{
			unordered_map<uint64_t, float> domain_scores;
			unordered_map<uint64_t, int> domain_counts;
			map<pair<uint64_t, uint64_t>, uint64_t> domain_unique;
			{
				for (const DomainLinkFullTextRecord &link : links) {

					if (domain_unique.count(make_pair(link.m_source_domain, link.m_target_domain)) == 0) {

						const float domain_score = expm1(25.0f*link.m_score) / 50.0f;
						domain_scores[link.m_target_domain] += domain_score;
						domain_counts[link.m_target_domain]++;
						domain_unique[make_pair(link.m_source_domain, link.m_target_domain)] = link.m_source_domain;

					}
				}
			}

			// Loop over the results and add the calculated domain scores.
			FullTextRecord *data = (FullTextRecord *)results->data_pointer();
			for (size_t i = 0; i < results->size(); i++) {
				const float domain_score = domain_scores[data[i].m_domain_hash];
				data[i].m_score += domain_score;
				applied_links += domain_counts[data[i].m_domain_hash];
			}
		}

		return applied_links;
	}

	template<typename DataRecord>
	size_t lower_bound(const DataRecord *data, size_t pos, size_t len, uint64_t value) {
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

	template<typename DataRecord>
	void value_intersection(const vector<FullTextResultSet<DataRecord> *> &result_sets, vector<int> sections, vector<DataRecord> &dest) {

		if (result_sets.size() == 0) {
			return;
		}

		size_t shortest_vector_position = 0;
		size_t shortest_len = SIZE_MAX;
		size_t iter_index = 0;
		for (FullTextResultSet<DataRecord> *result_set : result_sets) {
			if (shortest_len > result_set->size()) {
				shortest_len = result_set->size();
				shortest_vector_position = iter_index;
			}
			iter_index++;
		}

		vector<size_t> positions(result_sets.size(), 0);

		const DataRecord *shortest_data = result_sets[shortest_vector_position]->section_pointer(sections[shortest_vector_position]);

		const bool make_binsearch = false;

		while (positions[shortest_vector_position] < shortest_len) {

			bool all_equal = true;
			uint64_t value = shortest_data[positions[shortest_vector_position]].m_value;

			float score_sum = 0.0f;
			size_t iter_index = 0;
			for (FullTextResultSet<DataRecord> *result_set : result_sets) {
				const DataRecord *data_arr = result_set->section_pointer(sections[iter_index]);
				const size_t len = result_set->size();

				size_t *pos = &(positions[iter_index]);
				
				if (make_binsearch) {
					// this is a linear search.
					*pos = lower_bound(data_arr, *pos, len, value);
				} else {
					// this is a linear search.
					while (*pos < len && value > data_arr[*pos].m_value) {
						(*pos)++;
					}
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

	template<typename DataRecord>
	void calculate_intersection(const vector<FullTextResultSet<DataRecord> *> &result_sets, FullTextResultSet<DataRecord> *dest) {

		for (FullTextResultSet<DataRecord> *result : result_sets) {
			if (result->size() == 0) return;
		}

		vector<FullTextResultSet<DataRecord> *> sorted_result_sets(result_sets);

		sort(sorted_result_sets.begin(), sorted_result_sets.end(), [](const FullTextResultSet<DataRecord> *a, const FullTextResultSet<DataRecord> *b) {
			return a->total_num_results() < b->total_num_results();
		});

		const int schema_size = 2;
		const int schema[] = {0, 8};
		size_t schema_index = 0;

		vector<int> lengths;
		for (FullTextResultSet<DataRecord> *result : sorted_result_sets) {
			lengths.push_back(result->num_sections());
		}

		vector<vector<int>> partitions = Algorithm::incremental_partitions(lengths, 8);

		// First just try the top sections.
		{
			vector<DataRecord> result;
			value_intersection(sorted_result_sets, partitions[0], result);
			if (result.size() >= Config::result_limit) {
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
		vector<vector<DataRecord>> results(partitions.size());
		std::vector<std::future<vector<DataRecord>>> thread_results;
		for (const vector<int> &partition : partitions) {
			thread_results.emplace_back(pool.enqueue([sorted_result_sets, partition]() {
				vector<DataRecord> result;
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
		vector<DataRecord> merged_vec;
		Sort::merge_arrays(results, [](const DataRecord &a, const DataRecord &b) {
			return a.m_value < b.m_value;
		}, merged_vec);

		// copy.
		dest->copy_vector(merged_vec);
	}

	template<typename DataRecord>
	void sort_by_score(vector<DataRecord> &results) {
		sort(results.begin(), results.end(), [](const DataRecord &a, const DataRecord &b) {
			return a.m_score > b.m_score;
		});
	}

	/*
		puts the top n elements in the first n slots of results. Then sorts those top n elements by value.

		this function assumes that the input results are sorted by value! so it does nothing for n < results.size()
	*/
	template<typename DataRecord>
	void get_unsorted_results_with_top_scores(FullTextResultSet<DataRecord> *result, size_t n) {

		if (result->size() > n) {
			span<DataRecord> *arr = result->span_pointer();
			nth_element(arr->begin(), arr->begin() + (n - 1), arr->end(), SearchEngine::comparator_class<DataRecord>{});

			sort(arr->begin(), arr->begin() + n, [](const DataRecord &a, const DataRecord &b) {
				return a.m_value < b.m_value;
			});

			result->resize(n);
		}
	}

	template<typename DataRecord>
	bool result_has_many_domains(const FullTextResultSet<DataRecord> *results) {

		if (results->size() == 0) return false;

		const DataRecord *data = results->data_pointer();
		const uint64_t first_domain_hash = data[0].m_domain_hash;
		for (size_t i = 0; i < results->size(); i++) {
			if (data[i].m_domain_hash != first_domain_hash) {
				return true;
			}
		}

		return false;
	}

	template<typename DataRecord>
	void deduplicate_domains(FullTextResultSet<DataRecord> *results, size_t results_per_domain, size_t limit) {

		vector<DataRecord> deduplicate;
		unordered_map<uint64_t, size_t> domain_counts;
		DataRecord *records = results->data_pointer();
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

	template<typename DataRecord>
	vector<DataRecord> deduplicate_result_vector(const vector<DataRecord> &results, size_t limit) {

		vector<DataRecord> deduped;
		vector<DataRecord> non_deduped;

		map<uint64_t, int> d_count;
		for (const DataRecord &result : results) {
			if (d_count[result.m_domain_hash] < Config::deduplicate_domain_count) {
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
			vector<DataRecord> ret;
			Sort::merge_arrays(deduped, non_deduped, [] (const DataRecord &a, const DataRecord &b) {
				return a.m_score > b.m_score;
			}, ret);
			return ret;
		}

		deduped.resize(limit);

		return deduped;
	}

	template<typename DataRecord>
	vector<FullTextResultSet<DataRecord> *> search_shards(vector<FullTextResultSet<DataRecord> *> &result_sets,
		const vector<FullTextShard<DataRecord> *> &shards, const vector<string> &words) {

		assert(words.size() <= Config::query_max_words);
		assert(words.size() <= result_sets.size());

		vector<FullTextResultSet<DataRecord> *> result_vector;
		vector<string> searched_words;
		size_t word_id = 0;
		for (const string &word : words) {

			// One word should only be searched once.
			if (find(searched_words.begin(), searched_words.end(), word) != searched_words.end()) continue;
			
			searched_words.push_back(word);

			uint64_t word_hash = Hash::str(word);

			shards[word_hash % Config::ft_num_shards]->find(word_hash, result_sets[word_id]);

			result_vector.push_back(result_sets[word_id]);
			word_id++;
		}

		return result_vector;
	}

	template <typename DataRecord>
	void *search_partition(void *ptr) {

		struct SearchArguments<DataRecord> *input = (struct SearchArguments<DataRecord> *)ptr;

		reset_search_metric(*(input->metric));

		vector<string> words = Text::get_full_text_words(input->query, Config::query_max_words);
		if (words.size() == 0) return new FullTextResultSet<DataRecord>(0);

		if (input->storage->result_sets[input->partition_id].size() == 0) {
			LOG_ERROR("Non empty result_set passed to search_partition");
		}

		vector<FullTextResultSet<DataRecord> *> result_vector = search_shards<DataRecord>(input->storage->result_sets[input->partition_id],
			*(input->shards), words);

		FullTextResultSet<DataRecord> *flat_result;
		if (result_vector.size() > 1) {

			// We need to calculate the intersection of the given results.
			flat_result = input->storage->intersected_result[input->partition_id];
			flat_result->resize(0);
			calculate_intersection<DataRecord>(result_vector, flat_result);

			set_total_found<DataRecord>(result_vector, *(input->metric), (double)flat_result->size() / largest_result(result_vector));
		} else {
			flat_result = result_vector[0];
			set_total_found<DataRecord>(result_vector, *(input->metric), 1.0);
		}

		// Close file pointers.
		for (FullTextResultSet<DataRecord> *result_set : result_vector) {
			result_set->close_sections();
		}

		input->metric->m_link_domain_matches = apply_domain_link_scores(*(input->domain_links), flat_result);
		input->metric->m_link_url_matches = apply_link_scores(*(input->links), flat_result);

		get_unsorted_results_with_top_scores<DataRecord>(flat_result, input->limit);

		return (void *)flat_result;
	}

	template<typename DataRecord>
	vector<DataRecord> search_wrapper(SearchAllocation::Storage<DataRecord> *storage, vector<FullTextIndex<DataRecord> *> index_array,
		const vector<LinkFullTextRecord> &links, const vector<DomainLinkFullTextRecord> &domain_links, const string &query, size_t limit,
		struct SearchMetric &metric) {

		assert(index_array.size() == Config::ft_num_partitions);

		vector<struct SearchMetric> metrics_vector(index_array.size(), SearchMetric{});

		vector<pthread_t> threads;
		vector<struct SearchArguments<DataRecord>> args(index_array.size(), SearchArguments<DataRecord>{
			.query = query,
			.limit = limit
		});

		size_t partition_id = 0;
		for (FullTextIndex<DataRecord> *index : index_array) {
			pthread_t thread;

			args[partition_id].shards = index->shard_ptr();
			args[partition_id].metric = &metrics_vector[partition_id];
			args[partition_id].links = &links;
			args[partition_id].domain_links = &domain_links;
			args[partition_id].storage = storage;
			args[partition_id].partition_id = partition_id;

			pthread_create(&thread, NULL, search_partition<DataRecord>, (void *)&(args[partition_id]));
			threads.push_back(thread);
			partition_id++;
		}

		vector<span<DataRecord> *> result_arrays;
		vector<FullTextResultSet<DataRecord> *> result_pointers;
		for (auto &thread : threads) {
			FullTextResultSet<DataRecord> *result;
			pthread_join(thread, (void **)&result);
			result_pointers.push_back(result);
			result_arrays.push_back(result->span_pointer());
		}

		vector<DataRecord> complete_result;
		Sort::merge_arrays(result_arrays, [](const DataRecord &a, const DataRecord &b) {
			return a.m_value < b.m_value;
		}, complete_result);

		metric.m_total_found = 0;
		metric.m_link_domain_matches = 0;
		metric.m_link_url_matches = 0;
		for (const struct SearchMetric &m : metrics_vector) {
			metric.m_total_found += m.m_total_found;
			metric.m_link_domain_matches += m.m_link_domain_matches;
			metric.m_link_url_matches += m.m_link_url_matches;
		}

		// Sort.
		sort_by_score<DataRecord>(complete_result);

		return complete_result;
	}

	template<typename DataRecord>
	vector<DataRecord> search(SearchAllocation::Storage<DataRecord> *storage, vector<FullTextIndex<DataRecord> *> index_array,
		const vector<LinkFullTextRecord> &links, const vector<DomainLinkFullTextRecord> &domain_links, const string &query, size_t limit,
		struct SearchMetric &metric) {

		vector<DataRecord> complete_result = search_wrapper(storage, index_array, links, domain_links, query, limit, metric);
		
		if (complete_result.size() > limit) {
			complete_result.resize(limit);
		}

		return complete_result;
	}

}
