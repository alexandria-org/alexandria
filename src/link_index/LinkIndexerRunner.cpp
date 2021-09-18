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
#include "LinkIndexerRunner.h"
#include "LinkIndexer.h"
#include "DomainLinkIndexer.h"
#include <math.h>
#include "system/Logger.h"
#include "full_text/FullText.h"

LinkIndexerRunner::LinkIndexerRunner(const string &db_name, const string &domain_db_name, const string &hash_table_name,
	const string &domain_hash_table_name, const string &cc_batch, const SubSystem *sub_system, UrlToDomain *url_to_domain)
: m_cc_batch(cc_batch), m_db_name(db_name), m_domain_db_name(domain_db_name), m_hash_table_name(hash_table_name),
	m_domain_hash_table_name(domain_hash_table_name)
{
	m_sub_system = sub_system;
	m_url_to_domain = url_to_domain;
}

LinkIndexerRunner::~LinkIndexerRunner() {
}

void LinkIndexerRunner::run(const vector<string> local_files, size_t partition) {

	ThreadPool pool(Config::ft_num_threads_indexing);
	std::vector<std::future<string>> results;

	vector<vector<string>> chunks;
	vector_chunk<string>(local_files, ceil(local_files.size() / Config::ft_num_threads_indexing), chunks);

	int id = 1;
	for (const vector<string> &chunk : chunks) {

		results.emplace_back(
			pool.enqueue([this, chunk, id, partition] {
				return run_index_thread_with_local_files(chunk, id, partition);
			})
		);

		id++;

	}

	for(auto && result: results) {
		result.get();
	}

	merge();
	sort();
}

void LinkIndexerRunner::merge() {
	LogInfo("Merging...");

	const size_t merge_batch_size = 250;

	ThreadPool merge_pool(Config::li_num_threads_merging);
	std::vector<std::future<string>> merge_results;

	// Loop over shards and merge them.
	for (size_t shard_id = 0; shard_id < Config::ft_num_shards; ) {

		while (shard_id < Config::ft_num_shards && merge_results.size() < merge_batch_size) {

			merge_results.emplace_back(
				merge_pool.enqueue([this, shard_id] {
					return run_merge_thread(shard_id);
				})
			);

			shard_id++;

		}

		for (auto && result: merge_results) {
			result.get();
		}
		merge_results.clear();
	}

}

void LinkIndexerRunner::sort() {
	LogInfo("Sorting...");

	// Loop over hash table shards and merge them.
	for (size_t shard_id = 0; shard_id < Config::ht_num_shards; shard_id++) {
		HashTableShardBuilder *shard = new HashTableShardBuilder(m_hash_table_name, shard_id);
		shard->sort();
		delete shard;
	}
	for (size_t shard_id = 0; shard_id < Config::ht_num_shards; shard_id++) {
		HashTableShardBuilder *shard = new HashTableShardBuilder(m_domain_hash_table_name, shard_id);
		shard->sort();
		delete shard;
	}
}

string LinkIndexerRunner::run_index_thread_with_local_files(const vector<string> &local_files, int id, size_t partition) {

	vector<HashTableShardBuilder *> shard_builders;
	for (size_t i = 0; i < Config::ht_num_shards; i++) {
		shard_builders.push_back(new HashTableShardBuilder(m_hash_table_name, i));
	}

	vector<HashTableShardBuilder *> domain_shard_builders;
	for (size_t i = 0; i < Config::ht_num_shards; i++) {
		domain_shard_builders.push_back(new HashTableShardBuilder(m_domain_hash_table_name, i));
	}

	LinkIndexer indexer(id, m_db_name, m_sub_system, m_url_to_domain);
	DomainLinkIndexer domain_link_indexer(id, m_domain_db_name, m_sub_system, m_url_to_domain);
	size_t idx = 1;
	for (const string &local_file : local_files) {

		ifstream stream(local_file, ios::in);

		if (stream.is_open()) {
			{
				indexer.add_stream(shard_builders, stream, partition);
				indexer.write_cache(m_link_mutexes);
			}
			stream.clear();
			stream.seekg(0);
			{
				domain_link_indexer.add_stream(domain_shard_builders, stream, partition);
				domain_link_indexer.write_cache(m_domain_link_mutexes);
			}
		}

		stream.close();

		for (size_t i = 0; i < Config::ht_num_shards; i++) {
			if (shard_builders[i]->full()) {
				m_hash_table_mutexes[i].lock();
				shard_builders[i]->write();
				m_hash_table_mutexes[i].unlock();
			}
		}

		for (size_t i = 0; i < Config::ht_num_shards; i++) {
			if (domain_shard_builders[i]->full()) {
				m_domain_hash_table_mutexes[i].lock();
				domain_shard_builders[i]->write();
				m_domain_hash_table_mutexes[i].unlock();
			}
		}

		//LogInfo("Done " + to_string(idx) + " out of " + to_string(warc_paths.size()));

		idx++;
	}
	indexer.flush_cache(m_link_mutexes);
	domain_link_indexer.flush_cache(m_domain_link_mutexes);

	for (size_t i = 0; i < Config::ht_num_shards; i++) {
		m_hash_table_mutexes[i].lock();
		shard_builders[i]->write();
		m_hash_table_mutexes[i].unlock();
	}

	for (size_t i = 0; i < Config::ht_num_shards; i++) {
		m_domain_hash_table_mutexes[i].lock();
		domain_shard_builders[i]->write();
		m_domain_hash_table_mutexes[i].unlock();
	}

	for (HashTableShardBuilder *shard_builder : shard_builders) {
		delete shard_builder;
	}

	for (HashTableShardBuilder *domain_shard_builder : domain_shard_builders) {
		delete domain_shard_builder;
	}

	return "";
}

string LinkIndexerRunner::run_merge_thread(size_t shard_id) {

	/*
	FullTextShardBuilder<FullTextRecord> adjustment_shard("adjustments", shard_id);
	adjustment_shard.merge();

	FullTextShardBuilder<FullTextRecord> domain_adjustment_shard("domain_adjustments", shard_id);
	domain_adjustment_shard.merge();*/

	{
		FullTextShardBuilder<LinkFullTextRecord> shard(m_db_name, shard_id);
		shard.merge();
	}

	{
		FullTextShardBuilder<LinkFullTextRecord> shard(m_domain_db_name, shard_id);
		shard.merge();
	}

	return "";
}

string LinkIndexerRunner::run_merge_adjustments_thread(const FullTextIndexer *indexer, size_t shard_id) {

	/*
	FullTextShardBuilder<FullTextRecord> shard1("main_index", shard_id);
	FullTextShardBuilder<FullTextRecord> shard2("adjustments", shard_id);
	FullTextShardBuilder<FullTextRecord> shard3("domain_adjustments", shard_id);

	shard1.merge_domain(shard2, shard3, indexer->url_to_domain());

	LogInfo("Merged " + to_string(shard_id));

	return shard1.filename();*/
	return "";
}

