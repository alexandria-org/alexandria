
#include "FullTextIndex.h"

FullTextIndex::FullTextIndex(const string &db_name)
: m_db_name(db_name)
{
	for (size_t bucket_id = 0; bucket_id < FT_NUM_BUCKETS; bucket_id++) {
		m_buckets.push_back(new FullTextBucket(m_db_name, bucket_id, shard_ids_for_bucket(bucket_id)));
	}
}

FullTextIndex::~FullTextIndex() {
	for (FullTextBucket *bucket : m_buckets) {
		delete bucket;
	}
}

void FullTextIndex::wait_for_start() {
	usleep(1000000);
}

vector<FullTextResult> FullTextIndex::search_word(const string &word) {
	uint64_t word_hash = m_hasher(word);
	FullTextBucket *bucket = bucket_for_hash(word_hash);

	vector<FullTextResult> results = bucket->find(word_hash);
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
		FullTextBucket *bucket = bucket_for_hash(word_hash);
		vector<FullTextResult> results = bucket->find(word_hash);

		Profiler profiler1("Sorting results");

		sort(results.begin(), results.end(), [](const FullTextResult &a, const FullTextResult &b) {
			return a.m_value < b.m_value;
		});

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
	for (auto bucket : m_buckets) {
		size += bucket->disk_size();
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

vector<size_t> FullTextIndex::shard_ids_for_bucket(size_t bucket_id) {
	const size_t shards_per_bucket = FT_NUM_SHARDS / FT_NUM_BUCKETS;

	const size_t start = bucket_id * shards_per_bucket;
	const size_t end = start + shards_per_bucket;
	
	vector<size_t> shard_ids;
	for (size_t shard_id = start; shard_id < end; shard_id++) {
		shard_ids.push_back(shard_id);
	}

	return shard_ids;
}

FullTextBucket *FullTextIndex::bucket_for_hash(size_t hash) {
	const size_t shards_per_bucket = FT_NUM_SHARDS / FT_NUM_BUCKETS;
	size_t shard_id = hash % FT_NUM_SHARDS;
	size_t bucket_id = shard_id / shards_per_bucket;
	return m_buckets[bucket_id];
}

void FullTextIndex::sort_results(vector<FullTextResult> &results) {
	sort(results.begin(), results.end(), [](const FullTextResult a, const FullTextResult b) {
		return a.m_score > b.m_score;
	});
}
