
#include "LinkIndexer.h"
#include "system/Logger.h"
#include "text/Text.h"
#include <math.h>

LinkIndexer::LinkIndexer(int id, const string &db_name, const SubSystem *sub_system, UrlToDomain *url_to_domain)
: m_indexer_id(id), m_db_name(db_name), m_sub_system(sub_system)
{
	m_url_to_domain = url_to_domain;
	for (size_t shard_id = 0; shard_id < FT_NUM_SHARDS; shard_id++) {
		FullTextShardBuilder<LinkFullTextRecord> *shard_builder =
			new FullTextShardBuilder<LinkFullTextRecord>(m_db_name, shard_id, LI_INDEXER_CACHE_BYTES_PER_SHARD);
		m_shards.push_back(shard_builder);
	}
}

LinkIndexer::~LinkIndexer() {
	for (FullTextShardBuilder<LinkFullTextRecord> *shard : m_shards) {
		delete shard;
	}
}

void LinkIndexer::add_stream(vector<HashTableShardBuilder *> &shard_builders, basic_istream<char> &stream) {

	string line;
	while (getline(stream, line)) {
		vector<string> col_values;
		boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

		URL source_url(col_values[0], col_values[1]);
		float source_harmonic = source_url.harmonic(m_sub_system);

		URL target_url(col_values[2], col_values[3]);
		float target_harmonic = target_url.harmonic(m_sub_system);

		const string link_text = col_values[4].substr(0, 1000);

		const Link link(source_url, target_url, source_harmonic, target_harmonic);

		if (m_url_to_domain->has_url(target_url.hash())) {

			uint64_t link_hash = source_url.link_hash(target_url, link_text);

			shard_builders[link_hash % HT_NUM_SHARDS]->add(link_hash, source_url.str() + " links to " + target_url.str() + " with link text: " + link_text);

			const string link_colon = "link:" + target_url.host() + " link:www." + target_url.host(); 

			add_data_to_shards(link_hash, source_url, target_url, link_colon, source_harmonic);
			add_expanded_data_to_shards(link_hash, source_url, target_url, link_text, source_harmonic);			
		}

	}

	// sort shards.
	for (FullTextShardBuilder<LinkFullTextRecord> *shard : m_shards) {
		shard->sort_cache();
	}
}

void LinkIndexer::write_cache(mutex *write_mutexes) {
	{
		size_t idx = 0;
		for (FullTextShardBuilder<LinkFullTextRecord> *shard : m_shards) {
			if (shard->full()) {
				write_mutexes[idx].lock();
				shard->append();
				write_mutexes[idx].unlock();
			}

			idx++;
		}
	}
}

void LinkIndexer::flush_cache(mutex *write_mutexes) {
	{
		size_t idx = 0;
		for (FullTextShardBuilder<LinkFullTextRecord> *shard : m_shards) {
			write_mutexes[idx].lock();
			shard->append();
			write_mutexes[idx].unlock();

			idx++;
		}
	}
}

void LinkIndexer::add_data_to_shards(uint64_t link_hash, const URL &source_url, const URL &target_url,
	const string &link_text, float score) {

	vector<string> words = Text::get_full_text_words(link_text);
	for (const string &word : words) {

		const uint64_t word_hash = m_hasher(word);
		const size_t shard_id = word_hash % FT_NUM_SHARDS;

		m_shards[shard_id]->add(word_hash, LinkFullTextRecord{.m_value = link_hash, .m_score = score,
			.m_target_hash = target_url.hash()});
	}
}

void LinkIndexer::add_expanded_data_to_shards(uint64_t link_hash, const URL &source_url, const URL &target_url,
	const string &link_text, float score) {

	vector<string> words = Text::get_expanded_full_text_words(link_text);
	for (const string &word : words) {

		const uint64_t word_hash = m_hasher(word);
		const size_t shard_id = word_hash % FT_NUM_SHARDS;

		m_shards[shard_id]->add(word_hash, LinkFullTextRecord{.m_value = link_hash, .m_score = score,
			.m_target_hash = target_url.hash()});
	}
}

