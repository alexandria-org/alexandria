

#pragma once

#define HT_PORT_START 20000
#define HT_NUM_BUCKETS 8
#define HT_NUM_SHARDS 16384
#define HT_DATA_LENGTH 2000
#define HT_KEY_SIZE 8
#define HT_MESSAGE_ADD 1
#define HT_MESSAGE_FIND 2
#define HT_MESSAGE_STOP 3

#include <iostream>
#include <thread>
#include <vector>
#include <map>

#include "HashTableShard.h"

using namespace std;

class HashTableShard;

class HashTable {

public:

	HashTable();
	~HashTable();

	void add(uint64_t key, const string &value);
	string find(uint64_t key);

private:

	vector<HashTableShard *> m_shards;

};
