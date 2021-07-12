
#pragma once

#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <algorithm>

#include "LinkIndex.h"
#include "LinkResult.h"

using namespace std;

class LinkShardBuilder {

public:

	LinkShardBuilder(const string &db_name, size_t shard_id);
	~LinkShardBuilder();

	void add(uint64_t word_key, uint64_t link_hash, uint64_t source, uint64_t target, uint64_t source_domain,
		uint64_t target_domain, float score);
	void sort_cache();
	bool full() const;
	void append();
	void merge();

	string filename() const;
	string target_filename() const;
	void truncate();

	size_t disk_size() const;
	size_t cache_size() const;

	size_t count_keys(uint64_t for_key) const;

private:

	const string m_db_name;
	const size_t m_shard_id;
	mutable ifstream m_reader;
	ofstream m_writer;
	const size_t m_max_results = 10000000;

	map<uint64_t, vector<LinkResult>> m_cache;
	map<uint64_t, size_t> m_total_results;

	void save_file();

};
