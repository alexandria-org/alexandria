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
#include "url_link/indexer.h"
#include "logger/logger.h"
#include "text/text.h"
#include "full_text/full_text.h"
#include "full_text/url_to_domain.h"
#include <math.h>

using namespace std;

namespace url_link {

	indexer::indexer(int id, const string &db_name, const common::sub_system *sub_system, full_text::url_to_domain *url_to_domain)
	: m_indexer_id(id), m_db_name(db_name), m_sub_system(sub_system)
	{
		m_url_to_domain = url_to_domain;
		for (size_t shard_id = 0; shard_id < config::ft_num_shards; shard_id++) {
			full_text::full_text_shard_builder<::url_link::full_text_record> *shard_builder =
				new full_text::full_text_shard_builder<::url_link::full_text_record>(m_db_name, shard_id, config::li_cached_bytes_per_shard);
			m_shards.push_back(shard_builder);
		}
	}

	indexer::~indexer() {
		for (full_text::full_text_shard_builder<::url_link::full_text_record> *shard : m_shards) {
			delete shard;
		}
	}

	void indexer::add_stream(vector<hash_table::hash_table_shard_builder *> &shard_builders, basic_istream<char> &stream) {

		string line;
		while (getline(stream, line)) {
			vector<string> col_values;
			boost::algorithm::split(col_values, line, boost::is_any_of("\t"));

			URL target_url(col_values[2], col_values[3]);

			if (m_url_to_domain->has_url(target_url.hash())) {

				float target_harmonic = target_url.harmonic(m_sub_system);

				URL source_url(col_values[0], col_values[1]);
				float source_harmonic = source_url.harmonic(m_sub_system);


				const string link_text = col_values[4].substr(0, 1000);

				const ::url_link::link link(source_url, target_url, source_harmonic, target_harmonic);

				uint64_t link_hash = source_url.link_hash(target_url, link_text);

				add_expanded_data_to_shards(link_hash, source_url, target_url, link_text, source_harmonic);			
			}
		}

		// sort shards.
		for (auto shard : m_shards) {
			shard->sort_cache();
		}
	}

	void indexer::write_cache(vector<mutex> &write_mutexes) {
		{
			size_t idx = 0;
			for (auto shard : m_shards) {
				if (shard->full()) {
					write_mutexes[idx].lock();
					shard->append();
					write_mutexes[idx].unlock();
				}
				idx++;
			}
		}
	}

	void indexer::flush_cache(vector<mutex> &write_mutexes) {
		{
			size_t idx = 0;
			for (auto shard : m_shards) {
				write_mutexes[idx].lock();
				shard->append();
				write_mutexes[idx].unlock();
				idx++;
			}
		}
	}

	void indexer::add_expanded_data_to_shards(uint64_t link_hash, const URL &source_url, const URL &target_url,
		const string &link_text, float score) {

		vector<string> words = text::get_expanded_full_text_words(link_text);
		for (const string &word : words) {

			const uint64_t word_hash = m_hasher(word);
			const size_t shard_id = word_hash % config::ft_num_shards;

			m_shards[shard_id]->add(word_hash, ::url_link::full_text_record{.m_value = link_hash, .m_score = score,
				.m_source_domain = source_url.host_hash(), .m_target_hash = target_url.hash()});
		}
	}

}

