
#include "FullTextIndexer.h"
#include "system/Logger.h"
#include <math.h>

FullTextIndexer::FullTextIndexer(int id, const SubSystem *sub_system)
: m_indexer_id(id), m_sub_system(sub_system)
{
	for (size_t shard_id = 0; shard_id < FT_NUM_SHARDS; shard_id++) {
		const string file_name = "/mnt/"+(to_string(shard_id % 8))+"/output/precache_" + to_string(shard_id) + ".fti";
		FullTextShardBuilder *shard_builder = new FullTextShardBuilder(file_name);
		m_shards.push_back(shard_builder);
	}
}

FullTextIndexer::~FullTextIndexer() {
	for (FullTextShardBuilder *shard : m_shards) {
		delete shard;
	}
}

void FullTextIndexer::add_stream(vector<HashTableShardBuilder *> &shard_builders, basic_istream<char> &stream,
	const vector<size_t> &cols, const vector<uint32_t> &scores) {

	string line;
	while (getline(stream, line)) {
		vector<string> col_values;
		boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

		URL url(col_values[0]);
		int harmonic = url.harmonic(m_sub_system);

		uint64_t key_hash = m_hasher(col_values[0]);
		shard_builders[key_hash % HT_NUM_SHARDS]->add(key_hash, col_values[0]);

		size_t score_index = 0;
		for (size_t col_index : cols) {
			add_data_to_shards(col_values[0], col_values[col_index], scores[score_index]*harmonic);
			score_index++;
		}
	}

	// sort shards.
	for (FullTextShardBuilder *shard : m_shards) {
		shard->sort_cache();
	}
}

void FullTextIndexer::write_cache(mutex *write_mutexes) {
	size_t idx = 0;
	for (FullTextShardBuilder *shard : m_shards) {
		if (shard->full()) {
			write_mutexes[idx].lock();
			shard->append();
			write_mutexes[idx].unlock();
		}

		idx++;
	}
}

void FullTextIndexer::flush_cache(mutex *write_mutexes) {
	size_t idx = 0;
	for (FullTextShardBuilder *shard : m_shards) {
		write_mutexes[idx].lock();
		shard->append();
		write_mutexes[idx].unlock();

		idx++;
	}
}

void FullTextIndexer::add_data_to_shards(const string &key, const string &text, uint32_t score) {

	uint64_t key_hash = m_hasher(key);

	vector<string> words = get_full_text_words(text);
	for (const string &word : words) {

		const uint64_t word_hash = m_hasher(word);
		const size_t shard_id = word_hash % FT_NUM_SHARDS;

		m_shards[shard_id]->add(word_hash, key_hash, score);
	}
}
