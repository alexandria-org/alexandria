
#pragma once

#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cstring>

#include "FullTextResult.h"

#define FULL_TEXT_RECORD_SIZE 12
#define FULL_TEXT_MAX_KEYS 0xFFFFFFFF
#define FULL_TEXT_KEY_LEN 8
#define FULL_TEXT_RECORD_LEN 12
#define FULL_TEXT_SCORE_LEN 4

using namespace std;

class FullTextShard {

public:

	FullTextShard(const string &db_name, size_t shard);
	~FullTextShard();

	void add(uint64_t key, uint64_t value, uint32_t score);
	void sort_cache();
	vector<FullTextResult> find(uint64_t key) const;
	void save_file();
	void read_file();

	string filename() const;
	void truncate();

	size_t disk_size() const;
	size_t cache_size() const;

private:

	string m_db_name;
	string m_filename;

	unordered_map<uint64_t, vector<FullTextResult>> m_cache;

	// These variables always represent what is in the file.
	vector<uint64_t> m_keys;
	map<uint64_t, size_t> m_pos;
	map<uint64_t, size_t> m_len;
	
	size_t m_num_keys;
	size_t m_shard;

	size_t m_data_start;
	size_t m_pos_start;
	size_t m_len_start;
	
	const size_t m_buffer_len = 1000000*FULL_TEXT_RECORD_SIZE; // 1m elements, each is 12 bytes.
	char *m_buffer;

	vector<FullTextResult> find_cached(uint64_t key) const;
	vector<FullTextResult> find_stored(uint64_t key) const;
	void read_data_to_cache();

};
