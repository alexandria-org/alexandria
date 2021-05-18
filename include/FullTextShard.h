
#pragma once

#include <iostream>
#include <map>
#include <vector>
#include <fstream>
#include <algorithm>

#include "FullTextResult.h"

#define FULL_TEXT_RECORD_SIZE 12
#define FULL_TEXT_MAX_KEYS 0xFFFFFFFF
#define FULL_TEXT_KEY_LEN 8
#define FULL_TEXT_RECORD_LEN 12
#define FULL_TEXT_SCORE_LEN 4

using namespace std;

class FullTextShard {

public:

	FullTextShard(size_t shard);
	~FullTextShard();

	void add(uint64_t key, uint64_t value, uint32_t score);
	vector<FullTextResult> find(uint64_t key);
	void save_file();
	void read_file();

	string filename() const;

private:

	ifstream m_reader;
	ofstream m_writer;

	map<uint64_t, vector<FullTextResult>> m_cache;

	vector<uint64_t> m_keys;
	map<uint64_t, size_t> m_pos;
	map<uint64_t, size_t> m_len;
	
	size_t m_num_keys;
	size_t m_data_start;
	size_t m_shard;
	
	const size_t m_buffer_len = 1000000*FULL_TEXT_RECORD_SIZE; // 1m elements, each is 12 bytes.
	char *m_buffer;

	vector<FullTextResult> find_cached(uint64_t key);
	vector<FullTextResult> find_stored(uint64_t key);
	void read_data_to_cache();

};