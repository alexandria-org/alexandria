
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

	map<size_t, FullTextResult> result_map;

	set<size_t> intersection;
	bool first_word = true;
	for (const string &word : words) {
		uint64_t word_hash = m_hasher(word);
		size_t shard = word_hash % m_num_shards;
		const vector<FullTextResult> one_result = m_shards[shard]->find(word_hash);
		set<size_t> keys;
		for (const FullTextResult &result : one_result) {
			result_map[result.m_value] = result;
			keys.insert(result.m_value);
		}
		set<size_t> keys2 = intersection;
		if (first_word) {
			keys2 = keys;
			first_word = false;
		}
		intersection.clear();
		set_intersection(keys.begin(), keys.end(), keys2.begin(), keys2.end(),
			inserter(intersection, intersection.begin()));
	}

	vector<FullTextResult> results;
	for (size_t key : intersection) {
		results.push_back(result_map[key]);
	}

	return results;
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
	while (getline(stream, line)) {
		vector<string> col_values;
		boost::algorithm::split(col_values, line, boost::is_any_of("\t"));
		for (size_t col_index : cols) {
			add(col_values[0], col_values[col_index], scores[col_index]);
		}

	}

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

size_t FullTextIndex::size() const {
	size_t size = 0;
	for (size_t i = 0; i < m_num_shards; i++) {
		size += m_shards[i]->size();
	}

	return size;
}

void FullTextIndex::truncate() {
	for (size_t i = 0; i < m_num_shards; i++) {
		m_shards[i]->truncate();
	}
}
