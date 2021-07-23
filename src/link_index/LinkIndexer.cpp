
#include "LinkIndexer.h"
#include "system/Logger.h"
#include <math.h>

LinkIndexer::LinkIndexer(int id, const string &db_name, const SubSystem *sub_system, FullTextIndexer *ft_indexer)
: m_indexer_id(id), m_db_name(db_name), m_sub_system(sub_system)
{
	m_ft_indexer = ft_indexer;
	for (size_t shard_id = 0; shard_id < FT_NUM_SHARDS; shard_id++) {
		FullTextShardBuilder<LinkFullTextRecord> *shard_builder =
			new FullTextShardBuilder<LinkFullTextRecord>(m_db_name, shard_id, LI_INDEXER_CACHE_BYTES_PER_SHARD);
		m_shards.push_back(shard_builder);
	}

	for (size_t shard_id = 0; shard_id < FT_NUM_SHARDS; shard_id++) {
		FullTextShardBuilder<FullTextRecord> *shard_builder =
			new FullTextShardBuilder<FullTextRecord>("adjustments", shard_id, LI_INDEXER_CACHE_BYTES_PER_SHARD);
		m_adjustment_shards.push_back(shard_builder);
	}

	for (size_t shard_id = 0; shard_id < FT_NUM_SHARDS; shard_id++) {
		FullTextShardBuilder<FullTextRecord> *shard_builder =
			new FullTextShardBuilder<FullTextRecord>("domain_adjustments", shard_id, LI_INDEXER_CACHE_BYTES_PER_SHARD);
		m_domain_adjustment_shards.push_back(shard_builder);
	}
}

LinkIndexer::~LinkIndexer() {
	for (FullTextShardBuilder<LinkFullTextRecord> *shard : m_shards) {
		delete shard;
	}
	for (FullTextShardBuilder<FullTextRecord> *shard : m_adjustment_shards) {
		delete shard;
	}
	for (FullTextShardBuilder<FullTextRecord> *shard : m_domain_adjustment_shards) {
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

		const string link_text = col_values[4];

		const Link link(source_url, target_url, source_harmonic, target_harmonic);

		/*
		// Skip domain links for now
		if (m_ft_indexer->has_domain(target_url.host_hash())) {
			add_domain_link(col_values[4], link);
		}
		*/

		if (m_ft_indexer->has_key(target_url.hash())) {

			uint64_t link_hash = source_url.link_hash(target_url, link_text);

			add_url_link(col_values[4], link);

			shard_builders[link_hash % HT_NUM_SHARDS]->add(link_hash, source_url.str() + " links to " + target_url.str() + " with link text: " + col_values[4]);

			const string link_colon = "link:" + target_url.host() + " link:www." + target_url.host(); 

			add_data_to_shards(link_hash, source_url, target_url, link_colon, source_harmonic);
			add_data_to_shards(link_hash, source_url, target_url, col_values[4], source_harmonic);			
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

	{
		size_t idx = 0;
		for (FullTextShardBuilder<FullTextRecord> *shard : m_adjustment_shards) {
			if (shard->full()) {
				write_mutexes[idx].lock();
				shard->append();
				write_mutexes[idx].unlock();
			}

			idx++;
		}
	}

	{
		size_t idx = 0;
		for (FullTextShardBuilder<FullTextRecord> *shard : m_domain_adjustment_shards) {
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

	{
		size_t idx = 0;
		for (FullTextShardBuilder<FullTextRecord> *shard : m_adjustment_shards) {
			write_mutexes[idx].lock();
			shard->append();
			write_mutexes[idx].unlock();

			idx++;
		}
	}

	{
		size_t idx = 0;
		for (FullTextShardBuilder<FullTextRecord> *shard : m_domain_adjustment_shards) {
			write_mutexes[idx].lock();
			shard->append();
			write_mutexes[idx].unlock();

			idx++;
		}
	}
}

void LinkIndexer::add_data_to_shards(uint64_t link_hash, const URL &source_url, const URL &target_url,
	const string &link_text, float score) {

	vector<string> words = get_full_text_words(link_text);
	for (const string &word : words) {

		const uint64_t word_hash = m_hasher(word);
		const size_t shard_id = word_hash % FT_NUM_SHARDS;

		m_shards[shard_id]->add(word_hash, LinkFullTextRecord{.m_value = link_hash, .m_score = score,
			.m_source_hash = source_url.hash(), .m_target_hash = target_url.hash(),
			.m_source_domain = source_url.host_hash(), .m_target_domain = target_url.host_hash()});
	}
}

void LinkIndexer::add_domain_link(const string &link_text, const Link &link) {
	
	vector<string> words = get_full_text_words(link_text);

	for (const string &word : words) {

		const uint64_t word_hash = m_hasher(word);
		const size_t shard_id = word_hash % FT_NUM_SHARDS;

		m_domain_adjustment_shards[shard_id]->add(word_hash, FullTextRecord{.m_value = link.target_url().host_hash(),
			.m_score = link.domain_score()});
	}
}

void LinkIndexer::add_url_link(const string &link_text, const Link &link) {
	
	vector<string> words = get_full_text_words(link_text);

	for (const string &word : words) {

		const uint64_t word_hash = m_hasher(word);
		const size_t shard_id = word_hash % FT_NUM_SHARDS;

		m_adjustment_shards[shard_id]->add(word_hash, FullTextRecord{.m_value = link.target_url().hash(), .m_score = link.url_score(),
				.m_domain_hash = link.target_url().host_hash(), .m_url_len = link.target_url().size()});
	}
}
