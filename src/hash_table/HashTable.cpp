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

using namespace std;

HashTable::HashTable(const string &db_name)
: m_db_name(db_name)
{
	m_num_items = 0;
	for (size_t shard_id = 0; shard_id < Config::ht_num_shards; shard_id++) {
		auto shard = new HashTableShard(m_db_name, shard_id);
		m_num_items += shard->size();
		m_shards.push_back(shard);
	}

	LOG_INFO("HashTable contains " + to_string(m_num_items) + " (" + to_string((double)m_num_items/1000000000) + "b) urls");
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

void HashTable::print_all_items() const {
	for (HashTableShard *shard : m_shards) {
		shard->print_all_items();
	}
}

