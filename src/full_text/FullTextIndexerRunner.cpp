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
#include "FullTextIndexerRunner.h"
#include "FullText.h"
#include "FullTextIndexer.h"
#include <math.h>
#include "system/Logger.h"
#include "algorithm/Algorithm.h"

using namespace std;

FullTextIndexerRunner::FullTextIndexerRunner(const string &db_name, const string &hash_table_name, const string &cc_batch, const SubSystem *sub_system)
:
	m_cc_batch(cc_batch),
	m_db_name(db_name),
	m_hash_table_name(hash_table_name),
	m_hash_table_mutexes(Config::ft_num_shards),
	m_full_text_mutexes(Config::ft_num_shards)
{
	m_sub_system = sub_system;
	m_did_allocate_sub_system = false;
}

FullTextIndexerRunner::FullTextIndexerRunner(const string &db_name, const string &hash_table_name, const string &cc_batch)
:
	m_cc_batch(cc_batch),
	m_db_name(db_name),
	m_hash_table_name(hash_table_name),
	m_hash_table_mutexes(Config::ft_num_shards),
	m_full_text_mutexes(Config::ft_num_shards)
{
	m_sub_system = new SubSystem();
	m_did_allocate_sub_system = true;
}

FullTextIndexerRunner::FullTextIndexerRunner(const string &db_name, const string &hash_table_name, const SubSystem *sub_system)
:
	m_cc_batch("none"),
	m_db_name(db_name),
	m_hash_table_name(hash_table_name),
	m_hash_table_mutexes(Config::ft_num_shards),
	m_full_text_mutexes(Config::ft_num_shards)
{
	m_sub_system = sub_system;
	m_did_allocate_sub_system = false;
}

FullTextIndexerRunner::~FullTextIndexerRunner() {
	if (m_did_allocate_sub_system) {
		delete m_sub_system;
	}
}

void FullTextIndexerRunner::run(const vector<string> &local_files) {

	truncate_cache();

	ThreadPool pool(Config::ft_num_threads_indexing);
	std::vector<std::future<string>> results;

	vector<vector<string>> chunks;
	Algorithm::vector_chunk<string>(local_files, ceil(local_files.size() / Config::ft_num_threads_indexing) + 1, chunks);

	int id = 1;
	for (const vector<string> &chunk : chunks) {

		results.emplace_back(
			pool.enqueue([this, chunk, id] {
				return run_index_thread_with_local_files(chunk, id);
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

void FullTextIndexerRunner::merge() {
	LOG_INFO("Merging...");

	const size_t merge_batch_size = 500;

	ThreadPool merge_pool(Config::ft_num_threads_merging);
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

void FullTextIndexerRunner::sort() {
	LOG_INFO("Sorting...");

	// Loop over hash table shards and merge them.
	for (size_t shard_id = 0; shard_id < Config::ht_num_shards; shard_id++) {
		HashTableShardBuilder *shard = new HashTableShardBuilder(m_hash_table_name, shard_id);
		shard->sort();
		delete shard;
	}
}

void FullTextIndexerRunner::truncate_cache() {
	for (size_t shard_id = 0; shard_id < Config::ft_num_shards; shard_id++) {
		FullTextShardBuilder<struct FullTextRecord> *shard_builder =
			new FullTextShardBuilder<struct FullTextRecord>(m_db_name, shard_id);
		shard_builder->truncate_cache_files();
		delete shard_builder;
	}

}

string FullTextIndexerRunner::run_index_thread_with_local_files(const vector<string> &local_files, int id) {

	vector<HashTableShardBuilder *> shard_builders;
	for (size_t i = 0; i < Config::ht_num_shards; i++) {
		shard_builders.push_back(new HashTableShardBuilder(m_hash_table_name, i));
	}

	UrlToDomain url_to_domain("main_index");
	FullTextIndexer indexer(id, m_db_name, m_sub_system, &url_to_domain);
	size_t idx = 1;
	for (const string &local_file : local_files) {

		ifstream stream(local_file, ios::in);

		if (stream.is_open()) {
			indexer.add_stream(shard_builders, stream, {1, 2, 3, 4}, {10.0, 3.0, 2.0, 1}, m_cc_batch);
			indexer.write_cache(m_write_mutex);
		}

		stream.close();

		for (size_t i = 0; i < Config::ht_num_shards; i++) {
			if (shard_builders[i]->full()) {
				m_hash_table_mutexes[i].lock();
				shard_builders[i]->write();
				m_hash_table_mutexes[i].unlock();
			}
		}

		LOG_INFO("Done " + to_string(idx) + " out of " + to_string(local_files.size()) + " for " + m_db_name);

		idx++;
	}
	LOG_INFO("Done with all, flushing cache");
	indexer.flush_cache(m_full_text_mutexes);

	LOG_INFO("Done, writing hash table shards");
	for (size_t i = 0; i < Config::ht_num_shards; i++) {
		m_hash_table_mutexes[i].lock();
		shard_builders[i]->write();
		m_hash_table_mutexes[i].unlock();
	}

	LOG_INFO("Done, writing url_to_domain");
	m_write_url_to_domain_mutex.lock();
	indexer.write_url_to_domain();
	m_write_url_to_domain_mutex.unlock();

	LOG_INFO("Done, deleting hash table shards");
	for (HashTableShardBuilder *shard_builder : shard_builders) {
		delete shard_builder;
	}

	return "";
}

string FullTextIndexerRunner::run_merge_thread(size_t shard_id) {

	FullTextShardBuilder<struct FullTextRecord> shard(m_db_name, shard_id);
	shard.merge();

	return "";
}

