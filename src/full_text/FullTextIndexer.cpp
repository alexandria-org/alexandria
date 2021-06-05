
#include "FullTextIndexer.h"
#include <math.h>

FullTextIndexer::FullTextIndexer(HashTable *hash_table) {
	m_hash_table = hash_table;
	for (size_t shard_id = 0; shard_id < FT_NUM_SHARDS; shard_id++) {
		const string file_name = "/mnt/"+(to_string(shard_id % 8))+"/output/precache_" + to_string(shard_id) + ".fti";
		FullTextShardBuilder *shard_builder = new FullTextShardBuilder(file_name);
		shard_builder->truncate();
		m_shards.push_back(shard_builder);
	}
}

FullTextIndexer::~FullTextIndexer() {
	for (FullTextShardBuilder *shard : m_shards) {
		delete shard;
	}
}

void FullTextIndexer::add_stream(basic_istream<char> &stream, const vector<size_t> &cols,
	const vector<uint32_t> &scores) {

	string line;
	while (getline(stream, line)) {
		vector<string> col_values;
		boost::algorithm::split(col_values, line, boost::is_any_of("\t"));
		size_t score_index = 0;
		for (size_t col_index : cols) {
			add_data_to_shards(col_values[0], col_values[col_index], scores[score_index]);
			score_index++;
		}
	}

	// sort shards.
	for (FullTextShardBuilder *shard : m_shards) {
		shard->sort_cache();
	}
}

bool FullTextIndexer::should_write_cache() const {
	size_t total_size = 0;
	for (FullTextShardBuilder *shard : m_shards) {
		total_size += shard->cache_size();
	}
	return total_size > FT_INDEXER_MAX_CACHE_SIZE;
}

void FullTextIndexer::write_cache() {
	for (FullTextShardBuilder *shard : m_shards) {
		shard->append();
	}
}

void FullTextIndexer::add_data_to_shards(const string &key, const string &text, uint32_t score) {

	uint64_t key_hash = m_hasher(key);

	m_hash_table->add(key_hash, key);

	vector<string> words = get_full_text_words(text);
	for (const string &word : words) {

		const uint64_t word_hash = m_hasher(word);
		const size_t shard_id = word_hash % FT_NUM_SHARDS;

		m_shards[shard_id]->add(word_hash, key_hash, score);
	}
}
