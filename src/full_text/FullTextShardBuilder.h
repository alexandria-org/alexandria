
#pragma once

#include <iostream>
#include <map>
#include <unordered_map>
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

class FullTextShardBuilder {

public:

	FullTextShardBuilder(const string &file_name);
	~FullTextShardBuilder();

	void add(uint64_t key, uint64_t value, uint32_t score);
	void sort_cache();
	void append();
	void merge(const string &db_name, size_t shard_id);

	string filename() const;
	void truncate();

	size_t disk_size() const;
	size_t cache_size() const;

private:

	string m_filename;
	mutable ifstream m_reader;
	ofstream m_writer;
	const size_t m_max_results = 10000000;

	unordered_map<uint64_t, vector<FullTextResult>> m_cache;

	void save_file(const string &db_name, size_t shard_id);

};
