
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
	string find(uint64_t key) const;

private:

	// Maps keys to positions in file.
	unordered_map<uint64_t, size_t> m_pos;
	size_t m_shard_id;

	string filename() const;

};
