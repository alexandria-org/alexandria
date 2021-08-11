
#pragma once

#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <algorithm>
#include <stdio.h>
#include <string.h>

#include "HashTable.h"

using namespace std;

class HashTableShard {

public:

	HashTableShard(const string &db_name, size_t shard_id);
	~HashTableShard();

	string find(uint64_t key);

	string filename_data() const;
	string filename_pos() const;
	size_t shard_id() const;
	size_t size() const;
	void print_all_items();

private:

	const int m_significant = 12;

	// Maps keys to positions in file.
	unordered_map<uint64_t, pair<size_t, size_t>> m_pos;
	const string m_db_name;
	size_t m_shard_id;
	size_t m_size;
	bool m_loaded;

	void load();
	string data_at_position(size_t pos);

};
