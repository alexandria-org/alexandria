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
#include "full_text/FullTextIndex.h"
#include "full_text/FullTextRecord.h"
#include "full_text/FullTextShard.h"
#include "full_text/SearchMetric.h"
#include "link_index/LinkFullTextRecord.h"
#include "link_index/DomainLinkFullTextRecord.h"
#include "hash/Hash.h"
#include "sort/Sort.h"

using namespace std;

namespace SearchEngine {
	/*
		Public interface
	*/

	template<typename DataRecord>
	vector<DataRecord> search(vector<FullTextIndex<DataRecord> *> index_array, const string &query, size_t limit, struct SearchMetric &metric);

	vector<FullTextRecord> search_with_links(vector<FullTextIndex<FullTextRecord> *> index_array, const vector<LinkFullTextRecord> &links,
		const vector<DomainLinkFullTextRecord> &domain_links, const string &query, size_t limit, struct SearchMetric &metric);
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
	size_t apply_link_scores(const vector<LinkFullTextRecord> &links, FullTextResultSet<FullTextRecord> *results);
	size_t apply_domain_link_scores(const vector<DomainLinkFullTextRecord> &links, FullTextResultSet<FullTextRecord> *results);

	template<typename DataRecord>
	vector<size_t> value_intersection(const vector<FullTextResultSet<DataRecord> *> &result_sets,
		size_t &shortest_vector_position, vector<float> &scores, vector<vector<float>> &score_parts) {

		if (result_sets.size() == 0) return {};

		shortest_vector_position = 0;
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
		vector<size_t> result_ids;

		const DataRecord *shortest_data = result_sets[shortest_vector_position]->data_pointer();
		vector<float> score_vec(result_sets.size(), 0.0f);

		while (positions[shortest_vector_position] < shortest_len) {

			bool all_equal = true;
			uint64_t value = shortest_data[positions[shortest_vector_position]].m_value;

			float score_sum = 0.0f;
			size_t iter_index = 0;
			for (FullTextResultSet<DataRecord> *result_set : result_sets) {
				const DataRecord *data_arr = result_set->data_pointer();
				const size_t len = result_set->size();

				size_t *pos = &(positions[iter_index]);
				while (*pos < len && value > data_arr[*pos].m_value) {
					(*pos)++;
				}
				if (*pos < len && value == data_arr[*pos].m_value) {
					const float score = data_arr[*pos].m_score;
					score_sum += score;
					score_vec[iter_index] = score;
				}
				if (*pos < len && value < data_arr[*pos].m_value) {
					all_equal = false;
				}
				if (*pos >= len) {
					all_equal = false;
				}
				iter_index++;
			}
			if (all_equal) {
				scores.push_back(score_sum / result_sets.size());
				score_parts.push_back(score_vec);
				result_ids.push_back(positions[shortest_vector_position]);
			}

			positions[shortest_vector_position]++;
		}

		return result_ids;
	}

	template<typename DataRecord>
	void delete_result_vector(vector<FullTextResultSet<DataRecord> *> results) {

		for (FullTextResultSet<DataRecord> *result_object : results) {
			delete result_object;
		}
	}

	template<typename DataRecord>
	void limit_results(vector<DataRecord> &results, size_t limit) {
		if (results.size() > limit) {
			results.resize(limit);
		}
	}

	template<typename DataRecord>
	void sort_by_score(vector<DataRecord> &results) {
		sort(results.begin(), results.end(), [](const DataRecord &a, const DataRecord &b) {
			return a.m_score > b.m_score;
		});
	}

	template<typename DataRecord>
	void sort_by_value(vector<DataRecord> &results) {
		sort(results.begin(), results.end(), [](const DataRecord &a, const DataRecord &b) {
			return a.m_value < b.m_value;
		});
	}

	template<typename DataRecord>
	void deduplicate_by_value(vector<DataRecord> &results) {
		auto last = unique(results.begin(), results.end(),
			[](const DataRecord &a, const DataRecord &b) {
			return a.m_value == b.m_value;
		});
		results.erase(last, results.end());
	}

	template<typename DataRecord>
	void flatten_results(const FullTextResultSet<DataRecord> *results, const vector<float> &scores,	const vector<size_t> &indices,
		vector<DataRecord> &flat_result) {

		const DataRecord *record_arr = results->data_pointer();
		for (size_t i = 0; i < indices.size(); i++) {
			const DataRecord *record = &record_arr[indices[i]];
			const float score = scores[i];

			flat_result.push_back(*record);
			flat_result.back().m_score = score;
		}
	}

	template<typename DataRecord>
	FullTextResultSet<DataRecord> *merge_results_to_one(const vector<FullTextResultSet<DataRecord> *> results) {

		if (results.size() > 1) {
			size_t shortest_vector;
			vector<float> score_vector;
			vector<vector<float>> score_parts;
			vector<size_t> result_ids = value_intersection(results, shortest_vector, score_vector, score_parts);

			FullTextResultSet<DataRecord> *shortest = results[shortest_vector];
			FullTextResultSet<DataRecord> *merged = new FullTextResultSet<DataRecord>(result_ids.size());

			const DataRecord *source_arr = shortest->data_pointer();
			DataRecord *dest_arr = merged->data_pointer();
			for (size_t i = 0; i < result_ids.size(); i++) {
				const DataRecord *record = &source_arr[result_ids[i]];
				const float score = score_vector[i];

				dest_arr[i] = *record;
				dest_arr[i].m_score = score;
			}
			return merged;
		}

		return NULL;
	}

	template<typename DataRecord>
	void merge_results_to_vector(const vector<FullTextResultSet<DataRecord> *> results, vector<DataRecord> &merged) {
		merged.clear();
		if (results.size() > 1) {
			size_t shortest_vector;
			vector<float> score_vector;
			vector<vector<float>> score_parts;
			vector<size_t> result_ids = value_intersection(results, shortest_vector, score_vector, score_parts);

			{
				FullTextResultSet<DataRecord> *shortest = results[shortest_vector];
				flatten_results<DataRecord>(shortest, score_vector, result_ids, merged);
			}

		} else {
			/*
				This is just a copy from a c vector to a c++ vector. It takes time and we are not adding any value, should be optimized.
			*/
			const DataRecord *record_arr = results[0]->data_pointer();
			for (size_t i = 0; i < results[0]->size(); i++) {
				merged.push_back(record_arr[i]);
			}
		}
	}

	template<typename DataRecord>
	void merge_results_to_vector(const vector<FullTextResultSet<DataRecord> *> results, vector<DataRecord> &merged, vector<float> &score_vector) {
		merged.clear();
		if (results.size() > 1) {
			size_t shortest_vector;
			vector<vector<float>> score_parts;
			vector<size_t> result_ids = value_intersection(results, shortest_vector, score_vector, score_parts);

			{
				FullTextResultSet<DataRecord> *shortest = results[shortest_vector];
				flatten_results<DataRecord>(shortest, score_vector, result_ids, merged);
			}

		} else {
			const DataRecord *record_arr = results[0]->data_pointer();
			for (size_t i = 0; i < results[0]->size(); i++) {
				merged.push_back(record_arr[i]);
				score_vector.push_back(record_arr[i].m_score);
			}
		}
	}

	template<typename DataRecord>
	vector<DataRecord> get_results_with_top_scores_vector(const vector<DataRecord> results, vector<float> &scores, size_t limit) {

		if (results.size() > limit) {
			nth_element(scores.begin(), scores.begin() + (limit - 1), scores.end(), greater{});
			const float nth = scores[limit - 1];

			vector<DataRecord> top_results;
			for (const DataRecord &res : results) {
				if (res.m_score >= nth) {
					top_results.push_back(res);
				}
			}

			sort_by_score(top_results);
			limit_results(top_results, limit);
			return top_results;
		} else {
			return results;
		}
	}

	/*
		puts the top n elements in the first n slots of results. Then sorts those top n elements by value.

		this function assumes that the input results are sorted by value! so it does nothing for n < results.size()
	*/
	template<typename DataRecord>
	void get_results_with_top_scores(FullTextResultSet<DataRecord> *result, size_t n) {

		if (result->size() > n) {
			span<DataRecord> *arr = result->span_pointer();
			nth_element(arr->begin(), arr->begin() + (n - 1), arr->end(), SearchEngine::comparator_class<DataRecord>{});

			sort(arr->begin(), arr->begin() + n, [](const DataRecord &a, const DataRecord &b) {
				return a.m_score > b.m_score;
			});

			result->resize(n);
		} else {
			span<DataRecord> *arr = result->span_pointer();
			sort(arr->begin(), arr->end(), [](const DataRecord &a, const DataRecord &b) {
				return a.m_score > b.m_score;
			});
		}
	}

	template<typename DataRecord>
	vector<FullTextResultSet<DataRecord> *> search_shards(const vector<FullTextShard<DataRecord> *> &shards, const vector<string> &words) {

		vector<FullTextResultSet<DataRecord> *> result_vector;
		vector<string> searched_words;
		for (const string &word : words) {

			// One word should only be searched once.
			if (find(searched_words.begin(), searched_words.end(), word) != searched_words.end()) continue;
			
			searched_words.push_back(word);

			uint64_t word_hash = Hash::str(word);

			FullTextResultSet<DataRecord> *results = shards[word_hash % Config::ft_num_shards]->find(word_hash);

			result_vector.push_back(results);
		}

		return result_vector;
	}

	template <typename DataRecord>
	FullTextResultSet<DataRecord> *general_search(const vector<FullTextShard<DataRecord> *> &shards, const string &query, size_t limit,
        struct SearchMetric &metric) {

		reset_search_metric(metric);

		vector<string> words = Text::get_full_text_words(query);
		if (words.size() == 0) return new FullTextResultSet<DataRecord>(0);

		vector<FullTextResultSet<DataRecord> *> result_vector = search_shards<DataRecord>(shards, words);

		FullTextResultSet<DataRecord> *flat_result;
		if (result_vector.size() > 1) {
			flat_result = merge_results_to_one<DataRecord>(result_vector);

			set_total_found<DataRecord>(result_vector, metric, (double)flat_result->size() / largest_result(result_vector));
		} else {
			flat_result = result_vector[0];
			set_total_found<DataRecord>(result_vector, metric, 1.0);
			result_vector.clear();
		}
		get_results_with_top_scores<DataRecord>(flat_result, limit);

		delete_result_vector<DataRecord>(result_vector);

		return flat_result;
	}

	template<typename DataRecord>
	vector<DataRecord> search(vector<FullTextIndex<DataRecord> *> index_array, const string &query, size_t limit, struct SearchMetric &metric) {

		vector<future<FullTextResultSet<DataRecord> *>> futures;
		vector<struct SearchMetric> metrics_vector(index_array.size(), SearchMetric{});

		size_t idx = 0;
		for (FullTextIndex<DataRecord> *index : index_array) {
			future<FullTextResultSet<DataRecord> *> future = async(general_search<DataRecord>, index->shards(), query, limit,
				ref(metrics_vector[idx]));
			futures.push_back(move(future));
			idx++;
		}

		vector<span<DataRecord> *> result_arrays;
		for (auto &future : futures) {
			FullTextResultSet<DataRecord> *result = future.get();
			result_arrays.push_back(result->span_pointer());
		}

		vector<DataRecord> complete_result;
		Sort::merge_arrays(result_arrays, [](const DataRecord &a, const DataRecord &b) {
			return a.m_score > b.m_score;
		}, complete_result);

		metric.m_total_found = 0;
		for (const struct SearchMetric &m : metrics_vector) {
			metric.m_total_found += m.m_total_found;
		}

		if (complete_result.size() > limit) {
			complete_result.resize(limit);
		}

		return complete_result;
	}

}
