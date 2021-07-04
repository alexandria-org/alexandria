
#include "LinkIndexer.h"
#include "system/Logger.h"
#include <math.h>

LinkIndexer::LinkIndexer(int id, const SubSystem *sub_system, FullTextIndexer *ft_indexer)
: m_indexer_id(id), m_sub_system(sub_system)
{
	m_ft_indexer = ft_indexer;
	for (size_t shard_id = 0; shard_id < LI_NUM_SHARDS; shard_id++) {
		const string file_name = "/mnt/"+(to_string(shard_id % 8))+"/output/precache_" + to_string(shard_id) + ".fti";
		LinkShardBuilder *shard_builder = new LinkShardBuilder(file_name);
		m_shards.push_back(shard_builder);
	}
}

LinkIndexer::~LinkIndexer() {
	for (LinkShardBuilder *shard : m_shards) {
		delete shard;
	}
}

void LinkIndexer::add_stream(vector<HashTableShardBuilder *> &shard_builders, basic_istream<char> &stream) {

	string line;
	while (getline(stream, line)) {
		vector<string> col_values;
		boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

		URL source_url(col_values[0], col_values[1]);
		int source_harmonic = source_url.harmonic(m_sub_system);

		URL target_url(col_values[2], col_values[3]);
		int target_harmonic = target_url.harmonic(m_sub_system);

		const string link_text = col_values[4];

		const struct Link link{
			.source_url = source_url,
			.target_url = target_url,
			.target_host_hash = target_url.host_hash(),
			.source_harmonic = source_harmonic,
			.target_harmonic = target_harmonic
		};

		if (m_ft_indexer->has_domain(target_url.host_hash())) {
			adjust_score_for_domain_link(col_values[4], link);
		}

		if (m_ft_indexer->has_key(target_url.hash())) {

			uint64_t link_hash = source_url.link_hash(target_url);

			adjust_score_for_url_link(col_values[4], link);

			shard_builders[link_hash % HT_NUM_SHARDS]->add(link_hash, source_url.str() + " links to " + target_url.str() + " with link text: " + col_values[4]);

			const string link_colon = "link:" + target_url.host() + " link:www." + target_url.host(); 

			add_data_to_shards(link_hash, source_url, target_url, link_colon, source_harmonic);
			add_data_to_shards(link_hash, source_url, target_url, col_values[4], source_harmonic);			
		}

	}

	// sort shards.
	for (LinkShardBuilder *shard : m_shards) {
		shard->sort_cache();
	}
}

void LinkIndexer::write_cache(mutex *write_mutexes) {
	size_t idx = 0;
	for (LinkShardBuilder *shard : m_shards) {
		if (shard->full()) {
			write_mutexes[idx].lock();
			shard->append();
			write_mutexes[idx].unlock();
		}

		idx++;
	}
}

void LinkIndexer::flush_cache(mutex *write_mutexes) {
	size_t idx = 0;
	for (LinkShardBuilder *shard : m_shards) {
		write_mutexes[idx].lock();
		shard->append();
		write_mutexes[idx].unlock();

		idx++;
	}
}

void LinkIndexer::add_data_to_shards(uint64_t link_hash, const URL &source_url, const URL &target_url,
	const string &link_text, uint32_t score) {

	vector<string> words = get_full_text_words(link_text);
	for (const string &word : words) {

		const uint64_t word_hash = m_hasher(word);
		const size_t shard_id = word_hash % LI_NUM_SHARDS;

		m_shards[shard_id]->add(word_hash, link_hash, source_url.hash(), target_url.hash(), source_url.host_hash(),
			target_url.host_hash(), score);
	}
}

void LinkIndexer::adjust_score_for_domain_link(const string &link_text, const struct Link &link) {
	
	vector<string> words = get_full_text_words(link_text);

	for (const string &word : words) {
		const uint64_t word_hash = m_hasher(word);
		m_ft_indexer->add_domain_link(word_hash, link);
	}
}

void LinkIndexer::adjust_score_for_url_link(const string &link_text, const struct Link &link) {
	
	vector<string> words = get_full_text_words(link_text);

	for (const string &word : words) {
		const uint64_t word_hash = m_hasher(word);
		m_ft_indexer->add_url_link(word_hash, link);
	}
}
