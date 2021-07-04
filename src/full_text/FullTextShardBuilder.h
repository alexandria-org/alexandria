
#pragma once

#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <algorithm>

#include "FullTextIndex.h"
#include "FullTextResult.h"
#include "DomainAdjustment.h"
#include "URLAdjustment.h"
#include "parser/URL.h"

using namespace std;

class FullTextShardBuilder {

public:

	FullTextShardBuilder(const string &db_name, size_t shard_id);
	~FullTextShardBuilder();

	void add(uint64_t key, uint64_t value, uint32_t score);
	void sort_cache();
	bool full() const;
	void append();
	void merge();
	void apply_url_adjustment(URLAdjustment &adjustments);
	void apply_domain_adjustment(DomainAdjustment &adjustments);

	string filename() const;
	string target_filename() const;
	string adjustment_filename() const;
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

	const size_t m_max_num_keys = 10000000;
	const size_t m_buffer_len = m_max_num_keys*FULL_TEXT_RECORD_LEN; // 1m elements, each is 12 bytes.
	char *m_buffer;

	map<uint64_t, vector<FullTextResult>> m_cache;
	map<uint64_t, size_t> m_total_results;

	void save_file();

};
