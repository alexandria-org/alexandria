
#pragma once

#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <algorithm>

#include "FullTextIndexer.h"
#include "FullTextIndex.h"
#include "FullTextResult.h"
#include "AdjustmentList.h"
#include "parser/URL.h"

using namespace std;

class FullTextIndexer;

struct ShardInput {

	uint64_t key;
	uint64_t value;
	float score;

};

class FullTextShardBuilder {

public:

	FullTextShardBuilder(const string &db_name, size_t shard_id);
	~FullTextShardBuilder();

	void add(uint64_t key, uint64_t value, float score);
	void sort_cache();
	bool full() const;
	void append();
	void merge();
	bool should_merge();
	void add_adjustments(const AdjustmentList &adjustments);
	void merge_adjustments(const FullTextIndexer *indexer);

	string filename() const;
	string target_filename() const;
	string domain_adjustment_filename() const;
	string url_adjustment_filename() const;

	void truncate();
	void truncate_cache_files();

	size_t disk_size() const;
	size_t cache_size() const;

private:

	const string m_db_name;
	const size_t m_shard_id;

	mutable ifstream m_reader;
	ofstream m_writer;
	const size_t m_max_results = 10000000;

	const size_t m_max_cache_file_size = 300 * 1000 * 1000; // 200mb.
	const size_t m_max_cache_size = FT_INDEXER_CACHE_BYTES_PER_SHARD / sizeof(struct ShardInput);
	const size_t m_max_num_keys = 10000000;
	const size_t m_buffer_len = m_max_num_keys*FULL_TEXT_RECORD_LEN; // 1m elements, each is 12 bytes.
	char *m_buffer;

	vector<struct ShardInput *>m_input;
	size_t m_input_position;
	map<uint64_t, vector<FullTextResult>> m_cache;
	map<uint64_t, size_t> m_total_results;

	void read_data_to_cache();
	void save_file();

};
