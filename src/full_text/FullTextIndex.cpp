
#include "FullTextIndex.h"

FullTextIndex::FullTextIndex(const string &db_name)
: m_db_name(db_name)
{
	for (size_t shard_id = 0; shard_id < FT_NUM_SHARDS; shard_id++) {
		m_shards.push_back(new FullTextShard(m_db_name, shard_id));
	}
}

FullTextIndex::~FullTextIndex() {
	for (FullTextShard *shard : m_shards) {
		delete shard;
	}
}

vector<FullTextResult> FullTextIndex::search_word(const string &word) {
	uint64_t word_hash = m_hasher(word);

	vector<FullTextResult> results = m_shards[word_hash % FT_NUM_SHARDS]->find(word_hash);
	sort_results(results);
	return results;
}

vector<FullTextResult> FullTextIndex::search_phrase(const string &phrase) {

	vector<string> words = get_full_text_words(phrase);

	vector<string> searched_words;
	map<size_t, vector<FullTextResult>> result_map;
	map<size_t, vector<uint64_t>> values_map;
	
	bool first_word = true;
	size_t idx = 0;
	double total_time = 0.0;
	for (const string &word : words) {

		// One word should only be searched once.
		if (find(searched_words.begin(), searched_words.end(), word) != searched_words.end()) continue;
		
		searched_words.push_back(word);

		uint64_t word_hash = m_hasher(word);
		vector<FullTextResult> results = m_shards[word_hash % FT_NUM_SHARDS]->find(word_hash);

		for (size_t i = 1; i < results.size(); i++) {
			if (results[i].m_value < results[i-1].m_value) {
				cout << "RESULTS ARE NOT ORDERED" << endl;
			}
		}

		Profiler profiler1("Accumuating all results");

		vector<uint64_t> values;
		for (const FullTextResult &result : results) {
			values.push_back(result.m_value);
		}
		result_map[idx] = results;
		values_map[idx] = values;
		idx++;
		total_time += profiler1.get();
	}
	cout << "Profiler [Sorting results] took " << total_time << "ms" << endl;

	Profiler profiler2("value_intersection");
	size_t shortest_vector;
	vector<size_t> result_ids = value_intersection(values_map, shortest_vector);

	vector<FullTextResult> result;

	for (size_t result_id : result_ids) {
		result.push_back(result_map[shortest_vector][result_id]);
	}

	sort_results(result);

	return result;
}

size_t FullTextIndex::disk_size() const {
	size_t size = 0;
	for (auto shard : m_shards) {
		size += shard->disk_size();
	}
	return size;
}

vector<size_t> FullTextIndex::value_intersection(const map<size_t, vector<uint64_t>> &values_map,
	size_t &shortest_vector_position) const {
	
	if (values_map.size() == 0) return {};

	shortest_vector_position = 0;
	size_t shortest_len = SIZE_MAX;
	for (auto &iter : values_map) {
		if (shortest_len > iter.second.size()) {
			shortest_len = iter.second.size();
			shortest_vector_position = iter.first;
		}
	}

	vector<size_t> positions(values_map.size(), 0);
	vector<size_t> result_ids;
	while (positions[shortest_vector_position] < shortest_len) {

		bool all_equal = true;

		auto value_iter = values_map.find(shortest_vector_position);

		uint64_t value = value_iter->second.at(positions[shortest_vector_position]);

		for (auto &iter : values_map) {
			size_t *pos = &(positions[iter.first]);
			while (value > iter.second[*pos] && *pos < iter.second.size()) {
				(*pos)++;
			}
			if (value < iter.second[*pos] && *pos < iter.second.size()) {
				all_equal = false;
			}
			if (*pos >= iter.second.size()) {
				all_equal = false;
			}
		}
		if (all_equal) {
			result_ids.push_back(positions[shortest_vector_position]);
		}

		positions[shortest_vector_position]++;
	}

	return result_ids;
}

void FullTextIndex::sort_results(vector<FullTextResult> &results) {
	sort(results.begin(), results.end(), [](const FullTextResult a, const FullTextResult b) {
		return a.m_score > b.m_score;
	});
}
