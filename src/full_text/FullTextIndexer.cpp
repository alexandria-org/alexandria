
#include "FullTextIndexer.h"
#include "FullText.h"
#include "system/Logger.h"
#include "text/Text.h"
#include <math.h>

FullTextIndexer::FullTextIndexer(int id, const string &db_name, const SubSystem *sub_system, UrlToDomain *url_to_domain)
: m_indexer_id(id), m_db_name(db_name), m_sub_system(sub_system)
{
	m_url_to_domain = url_to_domain;
	for (size_t shard_id = 0; shard_id < Config::ft_num_shards; shard_id++) {
		FullTextShardBuilder<struct FullTextRecord> *shard_builder =
			new FullTextShardBuilder<struct FullTextRecord>(db_name, shard_id);
		m_shards.push_back(shard_builder);
	}
}

FullTextIndexer::~FullTextIndexer() {
	for (FullTextShardBuilder<struct FullTextRecord> *shard : m_shards) {
		delete shard;
	}
}

void FullTextIndexer::add_stream(vector<HashTableShardBuilder *> &shard_builders, basic_istream<char> &stream,
	const vector<size_t> &cols, const vector<float> &scores, size_t partition) {

	string line;
	while (getline(stream, line)) {
		vector<string> col_values;
		boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

		URL url(col_values[0]);

		if (FullText::should_index_url(url, partition)) {

			cout << "url " << url.str() << " was added to partition " << partition << " on node " << Config::node_id << "/" << Config::nodes_in_cluster << endl;

			float harmonic = url.harmonic(m_sub_system);

			m_url_to_domain->add_url(url.hash(), url.host_hash());

			uint64_t key_hash = url.hash();
			shard_builders[key_hash % Config::ht_num_shards]->add(key_hash, line);

			const string site_colon = "site:" + url.host() + " site:www." + url.host() + " " + url.host() + " " + url.domain_without_tld();

			size_t score_index = 0;
			map<uint64_t, float> word_map;

			add_data_to_word_map(word_map, site_colon, 20*harmonic);

			for (size_t col_index : cols) {
				add_expanded_data_to_word_map(word_map, col_values[col_index], scores[score_index]*harmonic);
				score_index++;
			}
			for (const auto &iter : word_map) {
				const uint64_t word_hash = iter.first;
				const size_t shard_id = word_hash % Config::ft_num_shards;
				m_shards[shard_id]->add(word_hash, FullTextRecord{.m_value = key_hash, .m_score = iter.second, .m_domain_hash = url.host_hash()});
			}
			word_map.clear();
		}
	}

}

void FullTextIndexer::add_link_stream(vector<HashTableShardBuilder *> &shard_builders, basic_istream<char> &stream) {

	string line;
	while (getline(stream, line)) {
		vector<string> col_values;
		boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

		URL source_url(col_values[0], col_values[1]);
		float source_harmonic = source_url.harmonic(m_sub_system);

		URL target_url(col_values[2], col_values[3]);

		uint64_t key_hash = target_url.hash();
		//shard_builders[key_hash % Config::ht_num_shards]->add(key_hash, target_url.str());

		const string site_colon = "link:" + target_url.host() + " link:www." + target_url.host(); 
		add_data_to_shards(target_url, site_colon, source_harmonic);

		add_data_to_shards(target_url, col_values[4], source_harmonic);
	}

	// sort shards.
	for (FullTextShardBuilder<struct FullTextRecord> *shard : m_shards) {
		shard->sort_cache();
	}
}

void FullTextIndexer::add_text(vector<HashTableShardBuilder *> &shard_builders, const string &key, const string &text,
		float score) {

	URL url(key);
	uint64_t key_hash = url.hash();
	shard_builders[key_hash % Config::ht_num_shards]->add(key_hash, key);

	add_data_to_shards(url, text, score);

	// sort shards.
	for (FullTextShardBuilder<struct FullTextRecord> *shard : m_shards) {
		shard->sort_cache();
	}
}

size_t FullTextIndexer::write_cache(mutex *write_mutexes) {
	size_t idx = 0;
	size_t ret = 0;
	for (FullTextShardBuilder<struct FullTextRecord> *shard : m_shards) {
		if (shard->full()) {
			if (write_mutexes[idx].try_lock()) {
				shard->append();
				ret++;
				write_mutexes[idx].unlock();
			}
		}

		idx++;
	}

	return ret;
}

size_t FullTextIndexer::write_large(mutex *write_mutexes) {
	size_t idx = 0;
	size_t ret = 0;
	for (FullTextShardBuilder<struct FullTextRecord> *shard : m_shards) {
		if (shard->should_merge()) {
			write_mutexes[idx].lock();
			if (shard->should_merge()) {
				cout << "MERGING shard: " << shard->filename() << endl;
				shard->merge();
				ret++;
			}
			write_mutexes[idx].unlock();
		}

		idx++;
	}

	return ret;
}

void FullTextIndexer::flush_cache(mutex *write_mutexes) {
	size_t idx = 0;
	for (FullTextShardBuilder<struct FullTextRecord> *shard : m_shards) {
		write_mutexes[idx].lock();
		shard->append();
		write_mutexes[idx].unlock();

		idx++;
	}
}

void FullTextIndexer::read_url_to_domain() {
	m_url_to_domain->read();
}

void FullTextIndexer::write_url_to_domain() {
	m_url_to_domain->write(m_indexer_id);
}

void FullTextIndexer::add_expanded_data_to_word_map(map<uint64_t, float> &word_map, const string &text, float score) const {

	vector<string> words = Text::get_expanded_full_text_words(text);
	map<uint64_t, uint64_t> uniq;
	for (const string &word : words) {
		const uint64_t word_hash = m_hasher(word);
		if (uniq.find(word_hash) == uniq.end()) {
			word_map[word_hash] += score;
			uniq[word_hash] = word_hash;
		}
	}
}

void FullTextIndexer::add_data_to_word_map(map<uint64_t, float> &word_map, const string &text, float score) const {

	vector<string> words = Text::get_full_text_words(text);
	map<uint64_t, uint64_t> uniq;
	for (const string &word : words) {
		const uint64_t word_hash = m_hasher(word);
		if (uniq.find(word_hash) == uniq.end()) {
			word_map[word_hash] += score;
			uniq[word_hash] = word_hash;
		}
	}
}

void FullTextIndexer::add_data_to_shards(const URL &url, const string &text, float score) {

	vector<string> words = Text::get_full_text_words(text);
	for (const string &word : words) {

		const uint64_t word_hash = m_hasher(word);
		const size_t shard_id = word_hash % Config::ft_num_shards;

		m_shards[shard_id]->add(word_hash, FullTextRecord{.m_value = url.hash(), .m_score = score, .m_domain_hash = url.host_hash()});
	}
}
