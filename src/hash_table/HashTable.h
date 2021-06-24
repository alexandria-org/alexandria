

#pragma once

#define HT_NUM_BUCKETS 8
#define HT_NUM_SHARDS 16384
#define HT_KEY_SIZE 8

#include <iostream>
#include <thread>
#include <vector>
#include <map>

#include "HashTableShard.h"
#include "system/SubSystem.h"
#include "system/ThreadPool.h"

using namespace std;

class HashTableShard;

class HashTable {

public:

	HashTable(const string &db_name);
	~HashTable();

	void add(uint64_t key, const string &value);
	void truncate();
	string find(uint64_t key);

	void upload(const SubSystem *sub_system);
	void download(const SubSystem *sub_system);
	void sort();

private:

	vector<HashTableShard *> m_shards;
	const string m_db_name;

	void run_upload_thread(const SubSystem *sub_system, const HashTableShard *shard);
	void run_download_thread(const SubSystem *sub_system, const HashTableShard *shard);

};
