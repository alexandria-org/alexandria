
#pragma once

#define FT_NUM_SHARDS 16384
#define FULL_TEXT_MAX_KEYS 0xFFFFFFFF
#define FULL_TEXT_KEY_LEN 8
#define FULL_TEXT_SCORE_LEN 4
#define FT_INDEXER_MAX_CACHE_GB 30
#define FT_NUM_THREADS_INDEXING 48
#define FT_NUM_THREADS_MERGING 24
#define FT_INDEXER_CACHE_BYTES_PER_SHARD ((FT_INDEXER_MAX_CACHE_GB * 1000ul*1000ul*1000ul) / (FT_NUM_SHARDS * FT_NUM_THREADS_INDEXING))

template<typename DataRecord> class FullTextIndex;

#include <iostream>
#include <vector>

#include <cmath>
#include "system/SubSystem.h"
#include "system/ThreadPool.h"
#include "FullTextRecord.h"
#include "FullTextShard.h"
#include "FullTextResultSet.h"
#include "SearchMetric.h"
#include "link_index/LinkFullTextRecord.h"
#include "text/Text.h"

#include "system/Logger.h"

using namespace std;

template<typename DataRecord>
class FullTextIndex {

public:
	FullTextIndex(const string &name);
	~FullTextIndex();

	vector<DataRecord> search_word(const string &word);

	void make_search_futures(const vector<string> &words, ThreadPool &pool, vector<future<FullTextResultSet<DataRecord> *>> &futures) const;

	vector<DataRecord> query_links(const FullTextIndex<LinkFullTextRecord> &link_fti, const string &phrase) const;
	vector<DataRecord> search_phrase(const string &phrase, int limit, size_t &total_found) const;
	vector<DataRecord> search_phrase(const FullTextIndex<LinkFullTextRecord> &link_fti, const string &phrase, int limit,
		size_t &total_found) const;
	vector<DataRecord> search_phrase(const FullTextIndex<LinkFullTextRecord> &link_fti, const string &phrase, int limit,
		struct SearchMetric &metric) const;
	vector<DataRecord> search_phrase_unsorted(const string &phrase, int limit, size_t &total_found) const;

	void deduplicate(const FullTextResultSet<DataRecord> *results, const vector<float> &scores, const vector<size_t> &indices,
		vector<DataRecord> &deduped_result, vector<float> &deduped_scores) const;
	void deduplicate_domains(const FullTextResultSet<DataRecord> *results, const vector<float> &scores, const vector<size_t> &indices,
		vector<DataRecord> &deduped_result, vector<float> &deduped_scores) const;

	size_t disk_size() const;

	void download(const SubSystem *sub_system);
	void upload(const SubSystem *sub_system);

	vector<size_t> value_intersection(const vector<FullTextResultSet<DataRecord> *> &result_sets,
		size_t &shortest_vector_position, vector<float> &scores) const;
	vector<size_t> value_intersection(const vector<FullTextResultSet<DataRecord> *> &result_sets,
		size_t &shortest_vector_position, vector<float> &scores, vector<vector<float>> &score_parts) const;
	
private:

	string m_db_name;
	hash<string> m_hasher;

	vector<FullTextShard<DataRecord> *> m_shards;

	void sort_results(vector<DataRecord> &results) const;
	void run_upload_thread(const SubSystem *sub_system, const FullTextShard<DataRecord> *shard);
	void run_download_thread(const SubSystem *sub_system, const FullTextShard<DataRecord> *shard);

};

template<typename DataRecord>
FullTextIndex<DataRecord>::FullTextIndex(const string &db_name)
: m_db_name(db_name)
{
	for (size_t shard_id = 0; shard_id < FT_NUM_SHARDS; shard_id++) {
		m_shards.push_back(new FullTextShard<DataRecord>(m_db_name, shard_id));
	}
}

template<typename DataRecord>
FullTextIndex<DataRecord>::~FullTextIndex() {
	for (FullTextShard<DataRecord> *shard : m_shards) {
		delete shard;
	}
}

template<typename DataRecord>
vector<DataRecord> FullTextIndex<DataRecord>::search_word(const string &word) {
	uint64_t word_hash = m_hasher(word);

	FullTextResultSet<DataRecord> *results = new FullTextResultSet<DataRecord>();
	m_shards[word_hash % FT_NUM_SHARDS]->find(word_hash, results);
	LogInfo("Searched for " + word + " and found " + to_string(results->len()) + " results");

	vector<DataRecord> ret;
	auto records = results->record_pointer();
	for (size_t i = 0; i < results->len(); i++) {
		ret.push_back(records[i]);
	}
	delete results;

	sort_results(ret);

	return ret;
}

template<typename DataRecord>
void FullTextIndex<DataRecord>::make_search_futures(const vector<string> &words, ThreadPool &pool,
	vector<future<FullTextResultSet<DataRecord> *>> &futures) const {

	vector<string> searched_words;
	for (const string &word : words) {

		// One word should only be searched once.
		if (find(searched_words.begin(), searched_words.end(), word) != searched_words.end()) continue;
		
		searched_words.push_back(word);

		uint64_t word_hash = m_hasher(word);
		FullTextResultSet<DataRecord> *results = new FullTextResultSet<DataRecord>();

		futures.emplace_back(
			pool.enqueue([this, word, word_hash, results] {
				Profiler profiler0("->find: " + word);
				LogInfo("Querying shard " + to_string(word_hash % FT_NUM_SHARDS) + " for word: " + word);
				m_shards[word_hash % FT_NUM_SHARDS]->find(word_hash, results);
				profiler0.stop();

				return results;
			})
		);

	}
}

/*
 * Returns a vector of LinkFullTextRecord with all the links sorted by ...
 * */
template<typename DataRecord>
vector<DataRecord> FullTextIndex<DataRecord>::query_links(const FullTextIndex<LinkFullTextRecord> &link_fti, const string &phrase) const {

	vector<string> words = Text::get_full_text_words(phrase);

	map<size_t, FullTextResultSet<LinkFullTextRecord> *> result_map;
	map<size_t, FullTextResultSet<LinkFullTextRecord> *> or_result_map;

	ThreadPool pool(24);
	std::vector<std::future<FullTextResultSet<LinkFullTextRecord> *>> thread_results;
	link_fti.make_search_futures(words, pool, thread_results);

	size_t idx = 0;
	Profiler profiler1("Running all queries");
	for (auto && thread_result : thread_results) {

		FullTextResultSet<LinkFullTextRecord> *results = thread_result.get();

		if (results->total_num_results() > results->len()) {
			or_result_map[idx] = results;
		} else {
			result_map[idx] = results;
		}
		idx++;
	}

	return {};
}

template<typename DataRecord>
vector<DataRecord> FullTextIndex<DataRecord>::search_phrase(const string &phrase, int limit, size_t &total_found) const {

	total_found = 0;

	vector<string> words = Text::get_full_text_words(phrase);

	if (words.size() == 0) return {};

	vector<string> searched_words;
	map<size_t, FullTextResultSet<DataRecord> *> result_map;
	map<size_t, FullTextResultSet<DataRecord> *> or_result_map;
	
	ThreadPool pool(24);
	std::vector<std::future<FullTextResultSet<DataRecord> *>> thread_results;
	for (const string &word : words) {

		// One word should only be searched once.
		if (find(searched_words.begin(), searched_words.end(), word) != searched_words.end()) continue;
		
		searched_words.push_back(word);

		uint64_t word_hash = m_hasher(word);
		FullTextResultSet<DataRecord> *results = new FullTextResultSet<DataRecord>();

		thread_results.emplace_back(
			pool.enqueue([this, word, word_hash, results] {
				Profiler profiler0("->find: " + word);
				LogInfo("Querying shard " + to_string(word_hash % FT_NUM_SHARDS) + " for word: " + word);
				m_shards[word_hash % FT_NUM_SHARDS]->find(word_hash, results);
				profiler0.stop();

				return results;
			})
		);

	}

	size_t idx = 0;
	Profiler profiler1("Running all queries");
	for (auto && thread_result : thread_results) {

		FullTextResultSet<DataRecord> *results = thread_result.get();

		LogInfo("Accumuating "+to_string(results->len())+" results for: " + words[idx]);

		if (results->total_num_results() > results->len()) {
			or_result_map[idx] = results;
		} else {
			result_map[idx] = results;
		}
		idx++;
	}
	double total_time = profiler1.get();

	if (result_map.size() == 0) {
		result_map = or_result_map;
		for (auto &iter : or_result_map) {
			if (iter.second->total_num_results() > total_found) {
				total_found = iter.second->total_num_results();
			}
		}
		or_result_map.clear();
	}

	vector<FullTextResultSet<DataRecord> *> result_vector;

	for (auto &iter : result_map) {
		result_vector.push_back(iter.second);
	}

	Profiler profiler2("value_intersection");
	size_t shortest_vector;
	vector<float> score_vector;
	vector<size_t> result_ids = value_intersection(result_vector, shortest_vector, score_vector);

	vector<DataRecord> result;

	FullTextResultSet<DataRecord> *shortest = result_vector[shortest_vector];
	DataRecord *record_arr = shortest->record_pointer();
	for (size_t i = 0; i < result_ids.size(); i++) {
		result.emplace_back(record_arr[result_ids[i]]);
		idx++;
	}
	profiler2.stop();

	if (total_found == 0) {
		total_found = score_vector.size();
	}

	for (auto &iter : result_map) {
		delete iter.second;
	}
	for (auto &iter : or_result_map) {
		delete iter.second;
	}

	Profiler profiler3("sorting results");
	if (result.size() > limit) {
		nth_element(score_vector.begin(), score_vector.begin() + (limit - 1), score_vector.end());
		const float nth = score_vector[limit - 1];

		vector<DataRecord> top_result;
		for (const DataRecord &res : result) {
			if (res.m_score >= nth) {
				top_result.push_back(res);
				if (top_result.size() >= limit) break;
			}
		}

		sort_results(top_result);

		return top_result;
	}

	sort_results(result);

	return result;
}

template<typename DataRecord>
vector<DataRecord> FullTextIndex<DataRecord>::search_phrase(const FullTextIndex<LinkFullTextRecord> &link_fti, const string &phrase, int limit,
		size_t &total_found) const {

	struct SearchMetric metric;
	vector<DataRecord> result = search_phrase(link_fti, phrase, limit, metric);

	total_found = metric.m_total_found;

	return result;
}

template<typename DataRecord>
vector<DataRecord> FullTextIndex<DataRecord>::search_phrase(const FullTextIndex<LinkFullTextRecord> &link_fti, const string &phrase, int limit,
		struct SearchMetric &metric) const {

	metric.m_total_found = 0;
	metric.m_total_links_found = 0;
	metric.m_links_handled = 0;
	metric.m_link_domain_matches = 0;
	metric.m_link_url_matches = 0;

	vector<string> words = Text::get_full_text_words(phrase);

	if (words.size() == 0) return {};

	map<size_t, FullTextResultSet<DataRecord> *> result_map;
	map<size_t, FullTextResultSet<DataRecord> *> or_result_map;

	Profiler profiler1("Running all queries");

	ThreadPool pool(24);
	std::vector<std::future<FullTextResultSet<DataRecord> *>> thread_results;
	make_search_futures(words, pool, thread_results);

	size_t total_links_found = 0;
	vector<LinkFullTextRecord> links = link_fti.search_phrase_unsorted(phrase, 10000, metric.m_total_links_found);
	metric.m_links_handled = links.size();

	size_t idx = 0;
	for (auto && thread_result : thread_results) {

		FullTextResultSet<DataRecord> *results = thread_result.get();

		LogInfo("Accumuating "+to_string(results->len())+" results for: " + words[idx]);

		//if (results->total_num_results() > results->len()) {
			//or_result_map[idx] = results;
		//} else {
			result_map[idx] = results;
		//}
		idx++;
	}

	double total_time = profiler1.get();

	if (result_map.size() == 0) {
		result_map = or_result_map;
		for (auto &iter : or_result_map) {
			if (iter.second->total_num_results() > metric.m_total_found) {
				metric.m_total_found = iter.second->total_num_results();
			}
		}
		or_result_map.clear();
	}

	vector<FullTextResultSet<DataRecord> *> result_vector;

	for (auto &iter : result_map) {
		result_vector.push_back(iter.second);
	}

	uint64_t *value_pt = result_vector[0]->value_pointer();

	for (size_t i = 0; i < result_vector[0]->len(); i++) {
		if (value_pt[i] == 11863130174689289975ul) {
			cout << "found it " << value_pt[i] << endl;
		}
	}

	Profiler profiler2("value_intersection");
	size_t shortest_vector;
	vector<float> score_vector;
	vector<vector<float>> score_parts;
	vector<size_t> result_ids = value_intersection(result_vector, shortest_vector, score_vector, score_parts);

	/*
	for (vector<float> &vec : score_parts) {
		for (float score : vec) {
			cout << "score_part: " << score << endl;
		}
	}*/

	vector<float> link_scores(result_ids.size(), 0.0f);

	FullTextResultSet<DataRecord> *shortest = result_vector[shortest_vector];
	if (typeid(DataRecord) == typeid(FullTextRecord)) {

		{
			const DataRecord *record_arr = shortest->record_pointer();
			size_t i = 0;
			size_t j = 0;
			const size_t host_bits = 20;
			while (i < links.size() && j < result_ids.size()) {

				const uint64_t host_part1 = (links[i].m_value >> (64 - host_bits)) << (64 - host_bits);
				const uint64_t host_part2 = (record_arr[result_ids[j]].m_value >> (64 - host_bits)) << (64 - host_bits);

				if (host_part1 < host_part2) {
					i++;
				} else if (host_part1 == host_part2) {

					const float domain_score = expm1(5*links[i].m_score) + 0.1;

					size_t jj = j;
					while (((record_arr[result_ids[jj]].m_value >> (64 - host_bits)) << (64 - host_bits)) == host_part2 && jj < result_ids.size()) {
						if (record_arr[result_ids[jj]].m_domain_hash == links[i].m_target_domain) {
							//cout << "Matched domain link and added score ["<< i <<"]: " << domain_score << endl;
							score_vector[jj] += domain_score;
							link_scores[jj] += domain_score;
							metric.m_link_domain_matches++;
						}
						jj++;
					}

					i++;
				} else {
					j++;
				}
			}
		}


		{
			sort(links.begin(), links.end(), [](const LinkFullTextRecord &a, const LinkFullTextRecord &b) {
				return a.m_target_hash < b.m_target_hash;
			});
			const DataRecord *record_arr = shortest->record_pointer();
			size_t i = 0;
			size_t j = 0;
			while (i < links.size() && j < result_ids.size()) {

				const uint64_t hash1 = links[i].m_target_hash;
				const uint64_t hash2 = record_arr[result_ids[j]].m_value;

				if (hash1 < hash2) {
					i++;
				} else if (hash1 == hash2) {
					const float url_score = expm1(10*links[i].m_score) + 0.1;
					//cout << "Matched link and added score: " << url_score << endl;
					score_vector[j] += url_score;
					link_scores[j] += url_score;
					metric.m_link_url_matches++;
					i++;
				} else {
					j++;
				}
			}
		}
	}

	vector<DataRecord> deduped_result;
	vector<float> deduped_scores;
	if (score_vector.size() >= 1000) {
		if (typeid(DataRecord) == typeid(FullTextRecord)) {
			deduplicate_domains(shortest, score_vector, result_ids, deduped_result, deduped_scores);
		} else {
			deduplicate(shortest, score_vector, result_ids, deduped_result, deduped_scores);
		}
	} else {
		deduplicate(shortest, score_vector, result_ids, deduped_result, deduped_scores);
	}

	profiler2.stop();

	if (metric.m_total_found == 0) {
		metric.m_total_found = score_vector.size();
	}

	for (auto &iter : result_map) {
		delete iter.second;
	}
	for (auto &iter : or_result_map) {
		delete iter.second;
	}

	Profiler profiler3("sorting results");
	if (deduped_result.size() > limit) {
		nth_element(deduped_scores.begin(), deduped_scores.begin() + (limit - 1), deduped_scores.end());
		const float nth = deduped_scores[limit - 1];

		vector<DataRecord> top_result;
		for (const DataRecord &res : deduped_result) {
			if (res.m_score >= nth) {
				top_result.push_back(res);
				if (top_result.size() >= limit) break;
			}
		}

		sort_results(top_result);

		return top_result;
	}

	sort_results(deduped_result);

	return deduped_result;
}

template<typename DataRecord>
vector<DataRecord> FullTextIndex<DataRecord>::search_phrase_unsorted(const string &phrase, int limit, size_t &total_found) const {

	total_found = 0;

	vector<string> words = Text::get_full_text_words(phrase);

	if (words.size() == 0) return {};

	vector<string> searched_words;
	map<size_t, FullTextResultSet<DataRecord> *> result_map;
	map<size_t, FullTextResultSet<DataRecord> *> or_result_map;
	
	ThreadPool pool(24);
	std::vector<std::future<FullTextResultSet<DataRecord> *>> thread_results;
	for (const string &word : words) {

		// One word should only be searched once.
		if (find(searched_words.begin(), searched_words.end(), word) != searched_words.end()) continue;
		
		searched_words.push_back(word);

		uint64_t word_hash = m_hasher(word);
		FullTextResultSet<DataRecord> *results = new FullTextResultSet<DataRecord>();

		thread_results.emplace_back(
			pool.enqueue([this, word, word_hash, results] {
				Profiler profiler0("->find: " + word);
				LogInfo("Querying shard " + to_string(word_hash % FT_NUM_SHARDS) + " for word: " + word);
				m_shards[word_hash % FT_NUM_SHARDS]->find(word_hash, results);
				profiler0.stop();

				return results;
			})
		);

	}

	size_t idx = 0;
	Profiler profiler1("Running all queries");
	for (auto && thread_result : thread_results) {

		FullTextResultSet<DataRecord> *results = thread_result.get();

		LogInfo("Accumuating "+to_string(results->len())+" results for: " + words[idx]);

		if (results->total_num_results() > results->len()) {
			or_result_map[idx] = results;
		} else {
			result_map[idx] = results;
		}
		idx++;
	}
	double total_time = profiler1.get();

	if (result_map.size() == 0) {
		result_map = or_result_map;
		for (auto &iter : or_result_map) {
			if (iter.second->total_num_results() > total_found) {
				total_found = iter.second->total_num_results();
			}
		}
		or_result_map.clear();
	}

	vector<FullTextResultSet<DataRecord> *> result_vector;

	for (auto &iter : result_map) {
		result_vector.push_back(iter.second);
	}

	Profiler profiler2("value_intersection");
	size_t shortest_vector;
	vector<float> score_vector;
	vector<size_t> result_ids = value_intersection(result_vector, shortest_vector, score_vector);

	vector<DataRecord> result;

	FullTextResultSet<DataRecord> *shortest = result_vector[shortest_vector];
	DataRecord *record_arr = shortest->record_pointer();
	for (size_t i = 0; i < result_ids.size(); i++) {
		result.emplace_back(record_arr[result_ids[i]]);
		idx++;
	}
	profiler2.stop();

	if (total_found == 0) {
		total_found = score_vector.size();
	}

	for (auto &iter : result_map) {
		delete iter.second;
	}
	for (auto &iter : or_result_map) {
		delete iter.second;
	}

	return result;
}

template<typename DataRecord>
void FullTextIndex<DataRecord>::deduplicate(const FullTextResultSet<DataRecord> *results, const vector<float> &scores,
		const vector<size_t> &indices, vector<DataRecord> &deduped_result, vector<float> &deduped_scores) const {

	const DataRecord *record_arr = results->record_pointer();
	for (size_t i = 0; i < indices.size(); i++) {
		const DataRecord *record = &record_arr[indices[i]];
		const float score = scores[i];

		deduped_result.push_back(*record);
		deduped_scores.push_back(score);
		deduped_result.back().m_score = score;
	}
}

template<typename DataRecord>
void FullTextIndex<DataRecord>::deduplicate_domains(const FullTextResultSet<DataRecord> *results, const vector<float> &scores,
		const vector<size_t> &indices, vector<DataRecord> &deduped_result, vector<float> &deduped_scores) const {

	const DataRecord *record_arr = results->record_pointer();
	unordered_map<uint64_t, float> max_scores;
	unordered_map<uint64_t, size_t> domain_position;
	for (size_t i = 0; i < indices.size(); i++) {
		const DataRecord *record = &record_arr[indices[i]];
		const float score = scores[i];
		auto iter = domain_position.find(record->m_domain_hash);
		if (iter == domain_position.end()) {
			domain_position[record->m_domain_hash] = deduped_result.size();
			deduped_result.push_back(*record);
			deduped_scores.push_back(score);
			deduped_result.back().m_score = score;
			max_scores[record->m_domain_hash] = score;
		} else {
			if (max_scores[record->m_domain_hash] < score) {
				deduped_result[iter->second] = *record;
				deduped_result[iter->second].m_score = score;
				max_scores[record->m_domain_hash] = score;
			}
		}
	}
}

template<typename DataRecord>
size_t FullTextIndex<DataRecord>::disk_size() const {
	size_t size = 0;
	for (auto shard : m_shards) {
		size += shard->disk_size();
	}
	return size;
}

template<typename DataRecord>
void FullTextIndex<DataRecord>::upload(const SubSystem *sub_system) {
	const size_t num_threads_uploading = 100;
	ThreadPool pool(num_threads_uploading);
	std::vector<std::future<void>> results;

	for (auto shard : m_shards) {
		results.emplace_back(
			pool.enqueue([this, sub_system, shard] {
				run_upload_thread(sub_system, shard);
			})
		);
	}

	for(auto && result: results) {
		result.get();
	}
}

template<typename DataRecord>
void FullTextIndex<DataRecord>::download(const SubSystem *sub_system) {
	const size_t num_threads_downloading = 100;
	ThreadPool pool(num_threads_downloading);
	std::vector<std::future<void>> results;

	for (auto shard : m_shards) {
		results.emplace_back(
			pool.enqueue([this, sub_system, shard] {
				run_download_thread(sub_system, shard);
			})
		);
	}

	for(auto && result: results) {
		result.get();
	}
}

template<typename DataRecord>
vector<size_t> FullTextIndex<DataRecord>::value_intersection(const vector<FullTextResultSet<DataRecord> *> &result_sets,
	size_t &shortest_vector_position, vector<float> &scores, vector<vector<float>> &score_parts) const {
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
vector<size_t> FullTextIndex<DataRecord>::value_intersection(const vector<FullTextResultSet<DataRecord> *> &result_sets,
	size_t &shortest_vector_position, vector<float> &scores) const {
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
			result_ids.push_back(positions[shortest_vector_position]);
		}

		positions[shortest_vector_position]++;
	}

	return result_ids;
}

template<typename DataRecord>
void FullTextIndex<DataRecord>::sort_results(vector<DataRecord> &results) const {
	sort(results.begin(), results.end(), [](const DataRecord a, const DataRecord b) {
		return a.m_score > b.m_score;
	});
}

template<typename DataRecord>
void FullTextIndex<DataRecord>::run_upload_thread(const SubSystem *sub_system, const FullTextShard<DataRecord> *shard) {
	ifstream infile(shard->filename());
	if (infile.is_open()) {
		const string key = "full_text/" + m_db_name + "/" + to_string(shard->shard_id()) + ".gz";
		sub_system->upload_from_stream("alexandria-index", key, infile);
	}
}

template<typename DataRecord>
void FullTextIndex<DataRecord>::run_download_thread(const SubSystem *sub_system, const FullTextShard<DataRecord> *shard) {
	ofstream outfile(shard->filename(), ios::binary | ios::trunc);
	if (outfile.is_open()) {
		const string key = "full_text/" + m_db_name + "/" + to_string(shard->shard_id()) + ".gz";
		sub_system->download_to_stream("alexandria-index", key, outfile);
	}
}

