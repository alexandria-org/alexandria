
#include "FullTextIndex.h"

FullTextIndex::FullTextIndex(const string &db_name)
: m_db_name(db_name)
{
	for (size_t i = 0; i < m_num_shards; i++) {
		m_shards.push_back(new FullTextShard(m_db_name, i));
	}
}
FullTextIndex::~FullTextIndex() {

}

vector<FullTextResult> FullTextIndex::search_word(const string &word) {
	uint64_t word_hash = m_hasher(word);
	size_t shard = word_hash % m_num_shards;
	return m_shards[shard]->find(word_hash);
}

vector<FullTextResult> FullTextIndex::search_phrase(const string &phrase) {

	vector<string> words = get_full_text_words(phrase);

	vector<string> searched_words;
	map<size_t, vector<FullTextResult>> result_map;
	map<size_t, vector<uint64_t>> values_map;
	
	bool first_word = true;
	size_t idx = 0;
	for (const string &word : words) {

		// One word should only be searched once.
		if (find(searched_words.begin(), searched_words.end(), word) != searched_words.end()) continue;
		
		searched_words.push_back(word);

		uint64_t word_hash = m_hasher(word);
		size_t shard = word_hash % m_num_shards;
		result_map[idx] = m_shards[shard]->find(word_hash);

		sort(result_map[idx].begin(), result_map[idx].end(), [](const FullTextResult &a, const FullTextResult &b) {
			return a.m_value < b.m_value;
		});

		vector<uint64_t> values;
		for (const FullTextResult &result : result_map[idx]) {
			values.push_back(result.m_value);
		}
		values_map[idx] = values;
		idx++;
	}

	size_t shortest_vector;
	vector<size_t> result_ids = value_intersection(values_map, shortest_vector);

	vector<FullTextResult> result;

	for (size_t result_id : result_ids) {
		result.push_back(result_map[shortest_vector][result_id]);
	}

	return result;
}

void FullTextIndex::add(const string &key, const string &text) {
	add(key, text, 1);
}

void FullTextIndex::add(const string &key, const string &text, uint32_t score) {
	uint64_t key_hash = m_hasher(key);

	vector<string> words = get_full_text_words(text);
	for (const string &word : words) {
		uint64_t word_hash = m_hasher(word);
		size_t shard = word_hash % m_num_shards;
		m_shards[shard]->add(word_hash, key_hash, score);
	}

}

void FullTextIndex::add_stream(basic_istream<char> &stream, const vector<size_t> &cols,
	const vector<uint32_t> &scores) {

	string line;
	Profiler profile("add_loop");
	while (getline(stream, line)) {
		vector<string> col_values;
		boost::algorithm::split(col_values, line, boost::is_any_of("\t"));
		for (size_t col_index : cols) {
			add(col_values[0], col_values[col_index], scores[col_index]);
		}
	}
	profile.stop();

	// sort shards.
	for (auto shard : m_shards) {
		shard->sort_cache();
	}
}

void FullTextIndex::save() {
	for (auto shard : m_shards) {
		shard->save_file();
	}
}

void FullTextIndex::truncate() {
	for (auto shard : m_shards) {
		shard->truncate();
	}
}

size_t FullTextIndex::disk_size() const {
	size_t size = 0;
	for (auto shard : m_shards) {
		size += shard->disk_size();
	}
	return size;
}

size_t FullTextIndex::cache_size() const {
	size_t size = 0;
	for (auto shard : m_shards) {
		size += shard->cache_size();
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