
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

	HashTableShard(size_t shard_id);
	~HashTableShard();

	void add(uint64_t key, const string &value);
	string find(uint64_t key);

	string filename_data() const;
	string filename_pos() const;
	size_t shard_id() const;

private:

	const int m_significant = 12;

	// Maps keys to positions in file.
	unordered_map<uint64_t, pair<size_t, size_t>> m_pos;
	size_t m_shard_id;
	bool m_loaded;

	void load();

};
