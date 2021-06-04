

#pragma once

#include <iostream>
#include <thread>
#include <vector>
#include <map>

#include "HashTableShard.h"
#include "HashTableMessage.h"

#define HT_PORT_START 20000
#define HT_NUM_SHARDS 32
#define HT_DATA_LENGTH 2000
#define HT_MESSAGE_ADD 1
#define HT_MESSAGE_FIND 2

using namespace std;

class HashTableMessage;

class HashTable {

public:

	HashTable();
	~HashTable();

	void add(const string &key, const string &value);
	string find(const string &key);

	size_t port() const { return m_port; };

private:

	int m_socket;
	int m_port;
	thread *m_thread;
	hash<string> m_hasher;

	string m_db_name;
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
