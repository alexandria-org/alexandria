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
#include "full_text_indexer_runner.h"
#include "full_text.h"
#include "full_text_indexer.h"
#include <math.h>
#include "logger/logger.h"
#include "algorithm/algorithm.h"

using namespace std;

namespace full_text {

	full_text_indexer_runner::full_text_indexer_runner(const string &db_name, const string &hash_table_name, const string &cc_batch,
			const common::sub_system *sub_system)
	:
		m_cc_batch(cc_batch),
		m_db_name(db_name),
		m_hash_table_name(hash_table_name),
		m_hash_table_mutexes(config::ft_num_shards),
		m_full_text_mutexes(config::ft_num_shards)
	{
		m_sub_system = sub_system;
		m_did_allocate_sub_system = false;
	}

	full_text_indexer_runner::full_text_indexer_runner(const string &db_name, const string &hash_table_name, const string &cc_batch)
	:
		m_cc_batch(cc_batch),
		m_db_name(db_name),
		m_hash_table_name(hash_table_name),
		m_hash_table_mutexes(config::ft_num_shards),
		m_full_text_mutexes(config::ft_num_shards)
	{
		m_sub_system = new common::sub_system();
		m_did_allocate_sub_system = true;
	}

	full_text_indexer_runner::full_text_indexer_runner(const string &db_name, const string &hash_table_name, const common::sub_system *sub_system)
	:
		m_cc_batch("none"),
		m_db_name(db_name),
		m_hash_table_name(hash_table_name),
		m_hash_table_mutexes(config::ft_num_shards),
		m_full_text_mutexes(config::ft_num_shards)
	{
		m_sub_system = sub_system;
		m_did_allocate_sub_system = false;
	}

	full_text_indexer_runner::~full_text_indexer_runner() {
		if (m_did_allocate_sub_system) {
			delete m_sub_system;
		}
	}

	void full_text_indexer_runner::run(const vector<string> &local_files) {

		truncate_cache();

		ThreadPool pool(config::ft_num_threads_indexing);
		std::vector<std::future<string>> results;

		vector<vector<string>> chunks;
		algorithm::vector_chunk<string>(local_files, ceil(local_files.size() / config::ft_num_threads_indexing) + 1, chunks);

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

	void full_text_indexer_runner::merge() {
		LOG_INFO("Merging...");

		const size_t merge_batch_size = 500;

		ThreadPool merge_pool(config::ft_num_threads_merging);
		std::vector<std::future<string>> merge_results;

		// Loop over shards and merge them.
		for (size_t shard_id = 0; shard_id < config::ft_num_shards; ) {

			while (shard_id < config::ft_num_shards && merge_results.size() < merge_batch_size) {

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

	void full_text_indexer_runner::sort() {
		LOG_INFO("Sorting...");

		// Loop over hash table shards and merge them.
		for (size_t shard_id = 0; shard_id < config::ht_num_shards; shard_id++) {
			hash_table::hash_table_shard_builder *shard = new hash_table::hash_table_shard_builder(m_hash_table_name, shard_id);
			shard->sort();
			delete shard;
		}
	}

	void full_text_indexer_runner::truncate_cache() {
		for (size_t shard_id = 0; shard_id < config::ft_num_shards; shard_id++) {
			full_text_shard_builder<struct full_text_record> *shard_builder =
				new full_text_shard_builder<struct full_text_record>(m_db_name, shard_id);
			shard_builder->truncate_cache_files();
			delete shard_builder;
		}

	}

	string full_text_indexer_runner::run_index_thread_with_local_files(const vector<string> &local_files, int id) {

		vector<hash_table::hash_table_shard_builder *> shard_builders;
		for (size_t i = 0; i < config::ht_num_shards; i++) {
			shard_builders.push_back(new hash_table::hash_table_shard_builder(m_hash_table_name, i));
		}

		url_to_domain url_to_domain(m_db_name);
		full_text_indexer indexer(id, m_db_name, m_sub_system, &url_to_domain);
		size_t idx = 1;
		for (const string &local_file : local_files) {

			ifstream stream(local_file, ios::in);

			if (stream.is_open()) {
				indexer.add_stream(shard_builders, stream, {1, 2, 3, 4}, {10.0, 3.0, 2.0, 1}, m_cc_batch, m_write_mutex);
			}

			stream.close();

			for (size_t i = 0; i < config::ht_num_shards; i++) {
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
		for (size_t i = 0; i < config::ht_num_shards; i++) {
			m_hash_table_mutexes[i].lock();
			shard_builders[i]->write();
			m_hash_table_mutexes[i].unlock();
		}

		LOG_INFO("Done, writing url_to_domain");
		m_write_url_to_domain_mutex.lock();
		indexer.write_url_to_domain();
		m_write_url_to_domain_mutex.unlock();

		LOG_INFO("Done, deleting hash table shards");
		for (hash_table::hash_table_shard_builder *shard_builder : shard_builders) {
			delete shard_builder;
		}

		return "";
	}

	string full_text_indexer_runner::run_merge_thread(size_t shard_id) {

		full_text_shard_builder<struct full_text_record> shard(m_db_name, shard_id);
		shard.merge();

		return "";
	}

}
