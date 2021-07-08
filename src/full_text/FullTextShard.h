
#pragma once

#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cstring>

#include "FullTextIndex.h"
#include "FullTextResult.h"
#include "FullTextResultSet.h"

using namespace std;

class FullTextShard {

public:

	FullTextShard(const string &db_name, size_t shard);
	~FullTextShard();

	void find(uint64_t key, FullTextResultSet *result_set);
	void read_keys();

	string filename() const;
	size_t shard_id() const;

	size_t disk_size() const;

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
	size_t m_total_start;
	
	const size_t m_max_num_keys = 10000000;
	const size_t m_buffer_len = m_max_num_keys*FULL_TEXT_RECORD_LEN; // 1m elements, each is 12 bytes.
	char *m_buffer;

};
