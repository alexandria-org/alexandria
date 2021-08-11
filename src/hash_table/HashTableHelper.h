
#pragma once

#include <iostream>
#include "hash_table/HashTable.h"
#include "hash_table/HashTableShardBuilder.h"

using namespace std;

namespace HashTableHelper {

	void truncate(const string &hash_table_name);
	vector<HashTableShardBuilder *> create_shard_builders(const string &hash_table_name);
	void delete_shard_builders(vector<HashTableShardBuilder *> &shards);
	void add_data(vector<HashTableShardBuilder *> &shards, uint64_t key, const string &value);
	void write(vector<HashTableShardBuilder *> &shards);
	void sort(vector<HashTableShardBuilder *> &shards);

}
