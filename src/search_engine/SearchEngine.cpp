
#include "SearchEngine.h"
#include "text/Text.h"

namespace SearchEngine {

	hash<string> hasher;

	void reset_search_metric(struct SearchMetric &metric) {
		metric.m_total_found = 0;
		metric.m_total_links_found = 0;
		metric.m_links_handled = 0;
		metric.m_link_domain_matches = 0;
		metric.m_link_url_matches = 0;
	}

	template<typename DataRecord>
	vector<FullTextResultSet<DataRecord> *> search_shards(const vector<FullTextShard<DataRecord> *> &shards, const vector<string> &words) {

		vector<FullTextResultSet<DataRecord> *> result_vector;
		vector<string> searched_words;
		for (const string &word : words) {

			// One word should only be searched once.
			if (find(searched_words.begin(), searched_words.end(), word) != searched_words.end()) continue;
			
			searched_words.push_back(word);

			uint64_t word_hash = hasher(word);
			FullTextResultSet<DataRecord> *results = new FullTextResultSet<DataRecord>();
			shards[word_hash % FT_NUM_SHARDS]->find(word_hash, results);

			result_vector.push_back(results);
		}

		return result_vector;
	}

	template<typename DataRecord>
	void set_total_found(const vector<FullTextResultSet<DataRecord> *> result_vector, struct SearchMetric &metric) {

		for (FullTextResultSet<DataRecord> *result : result_vector) {
			if (result->total_num_results() > metric.m_total_found) {
				metric.m_total_found = result->total_num_results();
			}
		}

	}

	template<typename DataRecord>
	vector<size_t> value_intersection(const vector<FullTextResultSet<DataRecord> *> &result_sets,
		size_t &shortest_vector_position, vector<float> &scores, vector<vector<float>> &score_parts) {

		Profiler value_intersection("FullTextIndex<DataRecord>::value_intersection");
		
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
			const DataRecord *record_arr = results[0]->record_pointer();
			for (size_t i = 0; i < results[0]->len(); i++) {
				merged.push_back(record_arr[i]);
			}
		}
	}

	template<typename DataRecord>
	void delete_result_vector(vector<FullTextResultSet<DataRecord> *> results) {

		for (FullTextResultSet<DataRecord> *result_object : results) {
			delete result_object;
		}
	}

	template<typename DataRecord>
	void sort_results_by_score(vector<DataRecord> &results) {
		sort(results.begin(), results.end(), [](const DataRecord &a, const DataRecord &b) {
			return a.m_score > b.m_score;
		});
	}

	template<typename DataRecord>
	bool result_has_many_domains(const vector<DataRecord> &results) {

		if (results.size() == 0) return false;

		const uint64_t first_domain_hash = results[0].m_domain_hash;
		for (const DataRecord &record : results) {
			if (record.m_domain_hash != first_domain_hash) {
				return true;
			}
		}

		return false;
	}

	template<typename DataRecord>
	vector<DataRecord> deduplicate_domains(const vector<DataRecord> results, size_t results_per_domain, size_t limit) {

		vector<DataRecord> deduplicate;
		unordered_map<uint64_t, size_t> domain_counts;
		for (const DataRecord &record : results) {
			if (deduplicate.size() >= limit) break;
			if (domain_counts[record.m_domain_hash] < results_per_domain) {
				deduplicate.push_back(record);
				domain_counts[record.m_domain_hash]++;
			}
		}

		return deduplicate;
	}

	template<typename DataRecord>
	vector<DataRecord> deduplicate_result(const vector<DataRecord> &results, size_t limit) {

		vector<DataRecord> deduped_result;
		bool has_many_domains = result_has_many_domains<DataRecord>(results);

		if (results.size() > 10 && has_many_domains) {
			deduped_result = deduplicate_domains<DataRecord>(results, 5, limit);
		} else {
			if (results.size() > limit) {
				deduped_result.insert(deduped_result.begin(), results.begin(), results.begin() + limit);
			} else {
				deduped_result = results;
			}
		}
		return deduped_result;
	}

	vector<FullTextRecord> search(const vector<FullTextShard<FullTextRecord> *> &shards, const vector<LinkFullTextRecord> &links,
		const string &query, size_t limit, struct SearchMetric &metric) {

		vector<FullTextRecord> flat_result;

		reset_search_metric(metric);

		vector<string> words = Text::get_full_text_words(query);
		if (words.size() == 0) return {};

		vector<FullTextResultSet<FullTextRecord> *> result_vector = search_shards<FullTextRecord>(shards, words);

		set_total_found<FullTextRecord>(result_vector, metric);

		merge_results_to_vector<FullTextRecord>(result_vector, flat_result);

		delete_result_vector<FullTextRecord>(result_vector);

		Scores::apply_link_scores(links, flat_result, metric);

		sort_results_by_score<FullTextRecord>(flat_result);

		vector<FullTextRecord> deduped_result = deduplicate_result<FullTextRecord>(flat_result, limit);

		return deduped_result;
	}

	vector<LinkFullTextRecord> search_links(const vector<FullTextShard<LinkFullTextRecord> *> &shards, const string &query, size_t limit,
		struct SearchMetric &metric) {

		vector<LinkFullTextRecord> flat_result;

		reset_search_metric(metric);

		vector<string> words = Text::get_full_text_words(query);
		if (words.size() == 0) return {};

		vector<FullTextResultSet<LinkFullTextRecord> *> result_vector = search_shards<LinkFullTextRecord>(shards, words);

		set_total_found<LinkFullTextRecord>(result_vector, metric);

		merge_results_to_vector<LinkFullTextRecord>(result_vector, flat_result);

		delete_result_vector<LinkFullTextRecord>(result_vector);

		return flat_result;
	}

}
