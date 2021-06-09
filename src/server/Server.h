
#pragma once

#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <algorithm>
#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include "abstract/TextBase.h"
#include "FullTextIndex.h"
#include "FullTextResult.h"
#include "FullTextShard.h"
#include "ServerMessage.h"


#define FT_PORT_START 8090
#define FT_MESSAGE_FIND 2
#define FT_MESSAGE_STOP 6
#define FT_MESSAGE_DISK_SIZE 7

using namespace std;

class Server : public TextBase {

public:

	Server(const string &db_name, size_t bucket_id, const vector<size_t> &shard_ids);
	~Server();

	vector<FullTextResult> find(uint64_t key);

	size_t disk_size();

	size_t port() const { return m_port; };

private:

	int m_socket;
	int m_port;
	thread *m_thread;
	hash<string> m_hasher;

	string m_db_name;
	size_t m_bucket_id;
	vector<size_t> m_shard_ids;
	map<size_t, FullTextShard *> m_shards;

	size_t m_first_shard_id;
	size_t m_last_shard_id;

	void run_server();
	ServerMessage send_message(const ServerMessage &message);
	bool read_socket(int socket);
	void load_shards();
	void close_shards();
};
