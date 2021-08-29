
#pragma once

#include <iostream>
#include <map>

#include "HashTable.h"

using namespace std;

class HashTableShardBuilder {

public:

	HashTableShardBuilder(const string &db_name, size_t shard_id);
	~HashTableShardBuilder();

	bool full() const;
	void write();
	void truncate();
	void sort();
	void optimize();

	void add(uint64_t key, const string &value);

	string filename_data() const;
	string filename_pos() const;
	string filename_data_tmp() const;
	string filename_pos_tmp() const;

private:

	map<uint64_t, string> m_cache;
	const string m_db_name;
	size_t m_shard_id;
	const size_t m_cache_limit;
	map<uint64_t, size_t> m_sort_pos;

	void read_keys();

};
