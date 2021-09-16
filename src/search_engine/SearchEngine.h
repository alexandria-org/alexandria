
#pragma once

#include <iostream>
#include <vector>
#include "full_text/FullTextIndex.h"
#include "full_text/FullTextRecord.h"
#include "full_text/FullTextShard.h"
#include "full_text/SearchMetric.h"
#include "link_index/LinkFullTextRecord.h"
#include "link_index/DomainLinkFullTextRecord.h"

using namespace std;

namespace SearchEngine {

	class comparator_class {
	public:
		// Comparator function
		bool operator()(DomainLinkFullTextRecord &a, DomainLinkFullTextRecord &b)
		{
			if (a.m_score == b.m_score) return a.m_value < b.m_value;
			return a.m_score > b.m_score;
		}
	};
	
	vector<FullTextRecord> search(const vector<FullTextShard<FullTextRecord> *> &shards, const vector<LinkFullTextRecord> &links,
		const string &query, size_t limit, struct SearchMetric &metric);

	vector<FullTextRecord> search_with_domain_links(const vector<FullTextShard<FullTextRecord> *> &shards, const vector<LinkFullTextRecord> &links,
		const vector<DomainLinkFullTextRecord> &domain_links, const string &query, size_t limit, struct SearchMetric &metric);

	vector<LinkFullTextRecord> search_links(const vector<FullTextShard<LinkFullTextRecord> *> &shards, const string &query, struct SearchMetric &metric);

	vector<FullTextRecord> search_index_array(vector<FullTextIndex<FullTextRecord> *> index_array, const vector<LinkFullTextRecord> &links,
		const string &query, size_t limit, struct SearchMetric &metric);

	vector<FullTextRecord> search_index_array(vector<FullTextIndex<FullTextRecord> *> index_array, const vector<LinkFullTextRecord> &links,
		const vector<DomainLinkFullTextRecord> &domain_links, const string &query, size_t limit, struct SearchMetric &metric);

	vector<LinkFullTextRecord> search_link_array(vector<FullTextIndex<LinkFullTextRecord> *> index_array, const string &query, size_t limit,
		struct SearchMetric &metric);

	vector<DomainLinkFullTextRecord> search_domain_link_array(vector<FullTextIndex<DomainLinkFullTextRecord> *> index_array, const string &query,
		size_t limit, struct SearchMetric &metric);

	/*
		Add scores for the given links to the result set. The links are assumed to be ordered by link.m_target_hash ascending.
	*/
	size_t apply_link_scores(const vector<LinkFullTextRecord> &links, vector<FullTextRecord> &results);
	size_t apply_domain_link_scores(const vector<DomainLinkFullTextRecord> &links, vector<FullTextRecord> &results);

	template<typename DataRecord>
	vector<size_t> value_intersection(const vector<FullTextResultSet<DataRecord> *> &result_sets,
		size_t &shortest_vector_position, vector<float> &scores, vector<vector<float>> &score_parts) {

		if (result_sets.size() == 0) return {};

		shortest_vector_position = 0;
		size_t shortest_len = SIZE_MAX;
		size_t iter_index = 0;
		for (FullTextResultSet<DataRecord> *result_set : result_sets) {
			if (shortest_len > result_set->len()) {
				shortest_len = result_set->len();
				shortest_vector_position = iter_index;
			}
			iter_index++;
		}

		vector<size_t> positions(result_sets.size(), 0);
		vector<size_t> result_ids;

		uint64_t *value_ptr = result_sets[shortest_vector_position]->value_pointer();
		vector<float> score_vec(result_sets.size(), 0.0f);

		while (positions[shortest_vector_position] < shortest_len) {

			bool all_equal = true;
			uint64_t value = value_ptr[positions[shortest_vector_position]];

			float score_sum = 0.0f;
			size_t iter_index = 0;
			for (FullTextResultSet<DataRecord> *result_set : result_sets) {
				const uint64_t *val_arr = result_set->value_pointer();
				const float *score_arr = result_set->score_pointer();
				const size_t len = result_set->len();
				size_t *pos = &(positions[iter_index]);
				while (*pos < len && value > val_arr[*pos]) {
					(*pos)++;
				}
				if (*pos < len && value == val_arr[*pos]) {
					score_sum += score_arr[*pos];
					score_vec[iter_index] = score_arr[*pos];
				}
				if (*pos < len && value < val_arr[*pos]) {
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

		const DataRecord *record_arr = results->record_pointer();
		for (size_t i = 0; i < indices.size(); i++) {
			const DataRecord *record = &record_arr[indices[i]];
			const float score = scores[i];

			flat_result.push_back(*record);
			flat_result.back().m_score = score;
		}
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
			const DataRecord *record_arr = results[0]->record_pointer();
			for (size_t i = 0; i < results[0]->len(); i++) {
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
			const DataRecord *record_arr = results[0]->record_pointer();
			for (size_t i = 0; i < results[0]->len(); i++) {
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
	void get_results_with_top_scores(vector<DataRecord> results, size_t n) {

		if (results.size() > n) {
			nth_element(results.begin(), results.begin() + (n - 1), results.end(), SearchEngine::comparator_class{});

			sort(results.begin(), results.begin() + (n - 1), [](const DataRecord &a, const DataRecord &b) {
				return a.m_value < b.m_value;
			});

		}

	}

}
