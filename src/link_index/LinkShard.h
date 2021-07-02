
#pragma once

#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cstring>

#include "LinkIndex.h"
#include "LinkResult.h"
#include "LinkResultSet.h"

using namespace std;

class LinkShard {

public:

	LinkShard(const string &db_name, size_t shard);
	~LinkShard();

	void find(uint64_t key, LinkResultSet *result_set);
	void read_keys();

	string filename() const;
	size_t shard_id() const;

	size_t disk_size() const;
	size_t cache_size() const;

private:

	string m_db_name;
	string m_filename;
	size_t m_shard_id;

	// These variables always represent what is in the file.
	vector<uint64_t> m_keys;

	bool m_keys_read;
	
	size_t m_num_keys;

	size_t m_data_start;
	size_t m_pos_start;
	size_t m_len_start;
	
	const size_t m_max_num_keys = 10000000;
	const size_t m_buffer_len = m_max_num_keys*LI_RECORD_LEN; // 1m elements, each is 12 bytes.
	char *m_buffer;

};
