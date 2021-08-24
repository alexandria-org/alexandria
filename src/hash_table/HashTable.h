

#pragma once

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
	size_t size() const;
	void print_all_items() const;

	void upload(const SubSystem *sub_system);
	void download(const SubSystem *sub_system);

private:

	vector<HashTableShard *> m_shards;
	const string m_db_name;
	size_t m_num_items;

	void run_upload_thread(const SubSystem *sub_system, const HashTableShard *shard);
	void run_download_thread(const SubSystem *sub_system, const HashTableShard *shard);

};
