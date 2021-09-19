/*
 * MIT License
 *
 * Alexandria.org
 *
 * Copyright (c) 2021 Josef Cullhed, <info@alexandria.org>, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "config.h"
#include "DomainLinkIndexer.h"
#include "system/Logger.h"
#include "text/Text.h"
#include "full_text/FullText.h"
#include <math.h>

DomainLinkIndexer::DomainLinkIndexer(int id, const string &db_name, const SubSystem *sub_system, UrlToDomain *url_to_domain)
: m_indexer_id(id), m_db_name(db_name), m_sub_system(sub_system)
{
	m_url_to_domain = url_to_domain;
	for (size_t partition = 0; partition < Config::ft_num_link_partitions; partition++) {
		for (size_t shard_id = 0; shard_id < Config::ft_num_shards; shard_id++) {
			FullTextShardBuilder<DomainLinkFullTextRecord> *shard_builder =
				new FullTextShardBuilder<DomainLinkFullTextRecord>(m_db_name + "_" + to_string(partition), shard_id,
					Config::li_cached_bytes_per_shard);
			m_shards[partition].push_back(shard_builder);
		}
	}
}

DomainLinkIndexer::~DomainLinkIndexer() {
	for (auto &iter : m_shards) {
		for (FullTextShardBuilder<DomainLinkFullTextRecord> *shard : iter.second) {
			delete shard;
		}
	}
}

void DomainLinkIndexer::add_stream(vector<HashTableShardBuilder *> &shard_builders, basic_istream<char> &stream) {

	string line;
	while (getline(stream, line)) {
		vector<string> col_values;
		boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

		URL target_url(col_values[2], col_values[3]);
		float target_harmonic = target_url.harmonic(m_sub_system);

		if (m_url_to_domain->has_domain(target_url.host_hash())) {

			URL source_url(col_values[0], col_values[1]);
			float source_harmonic = source_url.harmonic(m_sub_system);

			const string link_text = col_values[4].substr(0, 1000);

			const Link link(source_url, target_url, source_harmonic, target_harmonic);

			uint64_t link_hash = source_url.domain_link_hash(target_url, link_text);

			const size_t partition = link_hash % Config::ft_num_link_partitions;

#ifdef COMPILE_WITH_LINK_INDEX
			shard_builders[link_hash % Config::ht_num_shards]->add(link_hash, line);
#endif

			add_expanded_data_to_shards(partition, link_hash, source_url, target_url, link_text, source_harmonic);			
		}
	}

	// sort shards.
	for (auto &iter : m_shards) {
		for (FullTextShardBuilder<DomainLinkFullTextRecord> *shard : iter.second) {
			shard->sort_cache();
		}
	}
}

void DomainLinkIndexer::write_cache(mutex write_mutexes[Config::ft_num_link_partitions][Config::ft_num_shards]) {
	{
		for (auto &iter : m_shards) {
			size_t idx = 0;
			for (FullTextShardBuilder<DomainLinkFullTextRecord> *shard : iter.second) {
				if (shard->full()) {
					write_mutexes[iter.first][idx].lock();
					shard->append();
					write_mutexes[iter.first][idx].unlock();
				}

				idx++;
			}
		}
	}
}

void DomainLinkIndexer::flush_cache(mutex write_mutexes[Config::ft_num_link_partitions][Config::ft_num_shards]) {
	{
		for (auto &iter : m_shards) {
			size_t idx = 0;
			for (FullTextShardBuilder<DomainLinkFullTextRecord> *shard : iter.second) {
				write_mutexes[iter.first][idx].lock();
				shard->append();
				write_mutexes[iter.first][idx].unlock();

				idx++;
			}
		}
	}
}

void DomainLinkIndexer::add_data_to_shards(size_t partition, uint64_t link_hash, const URL &source_url, const URL &target_url,
	const string &link_text, float score) {

	vector<string> words = Text::get_full_text_words(link_text);
	for (const string &word : words) {

		const uint64_t word_hash = m_hasher(word);
		const size_t shard_id = word_hash % Config::ft_num_shards;

		m_shards[partition][shard_id]->add(word_hash, DomainLinkFullTextRecord{.m_value = link_hash, .m_score = score,
			.m_source_domain = source_url.host_hash(), .m_target_domain = target_url.host_hash()});
	}
}

void DomainLinkIndexer::add_expanded_data_to_shards(size_t partition, uint64_t link_hash, const URL &source_url, const URL &target_url,
	const string &link_text, float score) {

	vector<string> words = Text::get_expanded_full_text_words(link_text);
	for (const string &word : words) {

		const uint64_t word_hash = m_hasher(word);
		const size_t shard_id = word_hash % Config::ft_num_shards;

		m_shards[partition][shard_id]->add(word_hash, DomainLinkFullTextRecord{.m_value = link_hash, .m_score = score,
			.m_source_domain = source_url.host_hash(), .m_target_domain = target_url.host_hash()});
	}
}

