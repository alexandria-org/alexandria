
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

vector<FullTextResult> FullTextIndex::search(const string &word) {
	uint64_t word_hash = m_hasher(word);
	size_t shard = word_hash % m_num_shards;
	return m_shards[shard]->find(word_hash);
}

void FullTextIndex::add(const string &key, const string &text) {
	add(key, text, 1);
}

void FullTextIndex::add(const string &key, const string &text, uint32_t score) {
	uint64_t key_hash = m_hasher(key);

	vector<string> words = get_words(text);
	for (const string &word : words) {
		uint64_t word_hash = m_hasher(word);
		size_t shard = word_hash % m_num_shards;
		m_shards[shard]->add(word_hash, key_hash, score);
	}

}

void FullTextIndex::consume_stream(basic_iostream<char> &stream) {
	
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