
#pragma once

#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <algorithm>

using namespace std;

class HashTableShard {

public:

	HashTableShard(size_t shard_id);
	~HashTableShard();

	void add(uint64_t key, const string &value);
	string find(uint64_t key) const;

private:

	mutable ifstream m_reader;
	ofstream m_writer;

	// Maps keys to positions in file.
	unordered_map<uint64_t, size_t> m_pos;

};
