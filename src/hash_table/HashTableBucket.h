

#pragma once

#include <iostream>
#include <thread>
#include <vector>
#include <map>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <algorithm>

#include "HashTable.h"
#include "HashTableShard.h"
#include "HashTableMessage.h"

using namespace std;

class HashTableShard;

class HashTableBucket {

public:

	HashTableBucket(size_t bucket_id, const vector<size_t> &shard_ids);
	~HashTableBucket();

	void add(uint64_t key, const string &value);
	string find(uint64_t key);

	size_t port() const { return m_port; };

private:

	int m_socket;
	int m_port;
	thread *m_thread;
	hash<string> m_hasher;

	size_t m_bucket_id;
	vector<size_t> m_shard_ids;
	map<size_t, HashTableShard *> m_shards;

	size_t m_first_shard_id;
	size_t m_last_shard_id;

	void run_server();
	HashTableMessage send_message(const HashTableMessage &message);
	bool read_socket(int socket);
	void load_shards();
	void close_shards();
};
