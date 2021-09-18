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
#include "HashTable.h"
#include "HashTableShardBuilder.h"
#include "system/Logger.h"

HashTable::HashTable(const string &db_name)
: m_db_name(db_name)
{
	m_num_items = 0;
	for (size_t shard_id = 0; shard_id < Config::ht_num_shards; shard_id++) {
		auto shard = new HashTableShard(m_db_name, shard_id);
		m_num_items += shard->size();
		m_shards.push_back(shard);
	}

	LogInfo("HashTable contains " + to_string(m_num_items) + " (" + to_string((double)m_num_items/1000000000) + "b) urls");
}

HashTable::~HashTable() {

	for (HashTableShard *shard : m_shards) {
		delete shard;
	}
}

void HashTable::add(uint64_t key, const string &value) {

	const size_t shard_id = key % Config::ht_num_shards;
	HashTableShardBuilder builder(m_db_name, shard_id);

	builder.add(key, value);

	builder.write();

}

void HashTable::truncate() {
	if (m_num_items > 1000000) {
		cout << "Are you sure you want to truncate hash table containing " << m_num_items << " elements? [y/N]: ";
		string line;
		cin >> line;
		if (line != "y") {
			cout << "Exiting..." << endl;
			exit(0);
		}
	}
	for (size_t shard_id = 0; shard_id < Config::ht_num_shards; shard_id++) {
		HashTableShardBuilder builder(m_db_name, shard_id);
		builder.truncate();
	}
}

string HashTable::find(uint64_t key) {
	return m_shards[key % Config::ht_num_shards]->find(key);
}

size_t HashTable::size() const {
	return m_num_items;
}

void HashTable::upload(const SubSystem *sub_system) {
	const size_t num_threads_downloading = 100;
	ThreadPool pool(num_threads_downloading);
	std::vector<std::future<void>> results;

	for (auto shard : m_shards) {
		results.emplace_back(
			pool.enqueue([this, sub_system, shard] {
				run_upload_thread(sub_system, shard);
			})
		);
	}

	for(auto && result: results) {
		result.get();
	}
}

void HashTable::download(const SubSystem *sub_system) {
	const size_t num_threads_uploading = 100;
	ThreadPool pool(num_threads_uploading);
	std::vector<std::future<void>> results;

	for (auto shard : m_shards) {
		results.emplace_back(
			pool.enqueue([this, sub_system, shard] {
				run_download_thread(sub_system, shard);
			})
		);
	}

	for(auto && result: results) {
		result.get();
	}
}

void HashTable::print_all_items() const {
	for (HashTableShard *shard : m_shards) {
		shard->print_all_items();
	}
}

void HashTable::run_upload_thread(const SubSystem *sub_system, const HashTableShard *shard) {
	ifstream infile_data(shard->filename_data());
	if (infile_data.is_open()) {
		const string key = "hash_table/" + m_db_name + "/" + to_string(shard->shard_id()) + ".data.gz";
		sub_system->upload_from_stream("alexandria-index", key, infile_data);
	}

	ifstream infile_pos(shard->filename_pos());
	if (infile_pos.is_open()) {
		const string key = "hash_table/" + m_db_name + "/" + to_string(shard->shard_id()) + ".pos.gz";
		sub_system->upload_from_stream("alexandria-index", key, infile_pos);
	}
}

void HashTable::run_download_thread(const SubSystem *sub_system, const HashTableShard *shard) {
	ofstream outfile_data(shard->filename_data(), ios::binary | ios::trunc);
	if (outfile_data.is_open()) {
		const string key = "hash_table/" + m_db_name + "/" + to_string(shard->shard_id()) + ".data.gz";
		sub_system->download_to_stream("alexandria-index", key, outfile_data);
	}

	ofstream outfile_pos(shard->filename_pos(), ios::binary | ios::trunc);
	if (outfile_pos.is_open()) {
		const string key = "hash_table/" + m_db_name + "/" + to_string(shard->shard_id()) + ".pos.gz";
		sub_system->download_to_stream("alexandria-index", key, outfile_pos);
	}
}

