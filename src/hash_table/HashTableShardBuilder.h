
#pragma once

#include <iostream>
#include <map>

#include "HashTable.h"

using namespace std;

class HashTableShardBuilder {

public:

	HashTableShardBuilder(size_t shard_id);
	~HashTableShardBuilder();

	bool full() const;
	void write();

	void add(uint64_t key, const string &value);

private:

	map<uint64_t, string> m_cache;
	size_t m_shard_id;

	string filename() const;

};
