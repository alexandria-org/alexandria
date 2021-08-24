
#pragma once

#include "config.h"
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cstring>

template<typename DataRecord> class FullTextShard;

#include "FullTextIndex.h"
#include "FullTextResult.h"
#include "FullTextResultSet.h"

#include "system/Profiler.h"
#include "system/Logger.h"

using namespace std;

/*
File format explained

8 bytes = unsigned int number of keys = num_keys
8 bytes * num_keys = list of keys
8 bytes * num_keys = list of positions in file counted from data start
8 bytes * num_keys = list of lengths
[DATA]

*/

template<typename DataRecord>
class FullTextShard {

public:

	FullTextShard(const string &db_name, size_t shard_id);
	~FullTextShard();

	void find(uint64_t key, FullTextResultSet<DataRecord> *result_set);
	size_t total_num_results(uint64_t key);
	void read_keys();

	string mountpoint() const;
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
	const size_t m_buffer_len = m_max_num_keys * sizeof(DataRecord); // 1m elements
	char *m_buffer;

};

template<typename DataRecord>
FullTextShard<DataRecord>::FullTextShard(const string &db_name, size_t shard)
: m_shard_id(shard), m_db_name(db_name), m_keys_read(false) {
	m_filename = "/mnt/"+mountpoint()+"/full_text/fti_" + m_db_name + "_" + to_string(m_shard_id) + ".idx";
	m_buffer = new char[m_buffer_len];
}

template<typename DataRecord>
FullTextShard<DataRecord>::~FullTextShard() {
	delete m_buffer;
}

template<typename DataRecord>
void FullTextShard<DataRecord>::find(uint64_t key, FullTextResultSet<DataRecord> *result_set) {

	if (!m_keys_read) read_keys();

	auto iter = lower_bound(m_keys.begin(), m_keys.end(), key);

	if (iter == m_keys.end() || *iter > key) {
		return;
	}

	size_t key_pos = iter - m_keys.begin();

	ifstream reader(filename(), ios::binary);

	char buffer[64];

	// Read position and length.
	reader.seekg(m_pos_start + key_pos * 8, ios::beg);
	reader.read(buffer, 8);
	size_t pos = *((size_t *)(&buffer[0]));

	reader.seekg(m_len_start + key_pos * 8, ios::beg);
	reader.read(buffer, 8);
	size_t len = *((size_t *)(&buffer[0]));

	reader.seekg(m_total_start + key_pos * 8, ios::beg);
	reader.read(buffer, 8);
	size_t total_num_results = *((size_t *)(&buffer[0]));
	result_set->set_total_num_results(total_num_results);

	reader.seekg(m_data_start + pos, ios::beg);

	const size_t num_records = len / sizeof(DataRecord);

	result_set->allocate(num_records);

	uint64_t *value_res = result_set->value_pointer();
	float *score_res = result_set->score_pointer();
	DataRecord *record_res = result_set->record_pointer();

	size_t read_bytes = 0;
	size_t kk = 0;
	while (read_bytes < len) {
		const size_t bytes_left = len - read_bytes;
		const size_t max_read_len = min(m_buffer_len, bytes_left);
		reader.read(m_buffer, max_read_len);
		const size_t read_len = reader.gcount();
		read_bytes += read_len;

		size_t num_records_read = read_len / sizeof(DataRecord);
		for (size_t i = 0; i < num_records_read; i++) {
			DataRecord *item = (DataRecord *)&m_buffer[i * sizeof(DataRecord)];
			value_res[kk] = item->m_value;
			score_res[kk] = item->m_score;
			record_res[kk] = *item;
			kk++;
		}
	}
}

template<typename DataRecord>
size_t FullTextShard<DataRecord>::total_num_results(uint64_t key) {

	if (!m_keys_read) read_keys();

	auto iter = lower_bound(m_keys.begin(), m_keys.end(), key);

	if (iter == m_keys.end() || *iter > key) {
		return 0;
	}

	size_t key_pos = iter - m_keys.begin();

	ifstream reader(filename(), ios::binary);

	char buffer[64];

	// Read total num results.
	reader.seekg(m_total_start + key_pos * 8, ios::beg);
	reader.read(buffer, 8);
	size_t total_num_results = *((size_t *)(&buffer[0]));

	return total_num_results;
}

template<typename DataRecord>
void FullTextShard<DataRecord>::read_keys() {

	m_keys_read = true;

	m_keys.clear();
	m_num_keys = 0;

	char buffer[64];

	ifstream reader(filename(), ios::binary);

	if (!reader.is_open()) {
		m_num_keys = 0;
		m_data_start = 0;
		return;
	}

	reader.seekg(0, ios::end);
	size_t file_size = reader.tellg();
	if (file_size == 0) {
		m_num_keys = 0;
		m_data_start = 0;
		return;
	}

	reader.seekg(0, ios::beg);
	reader.read(buffer, 8);

	m_num_keys = *((uint64_t *)(&buffer[0]));

	if (m_num_keys > Config::ft_max_keys) {
		throw error("Number of keys in file exceeeds maximum: file: " + filename() + " num: " + to_string(m_num_keys));
	}

	char *vector_buffer = new char[m_num_keys * 8];

	// Read the keys.
	reader.read(vector_buffer, m_num_keys * 8);
	for (size_t i = 0; i < m_num_keys; i++) {
		m_keys.push_back(*((size_t *)(&vector_buffer[i*8])));
	}
	delete vector_buffer;

	m_pos_start = reader.tellg();


	m_len_start = m_pos_start + m_num_keys * 8;
	m_total_start = m_len_start + m_num_keys * 8;
	m_data_start = m_total_start + m_num_keys * 8;

}

template<typename DataRecord>
string FullTextShard<DataRecord>::mountpoint() const {
	hash<string> hasher;
	return to_string((hasher(m_db_name) + m_shard_id) % 8);
}

template<typename DataRecord>
string FullTextShard<DataRecord>::filename() const {
	return m_filename;
}

template<typename DataRecord>
size_t FullTextShard<DataRecord>::shard_id() const {
	return m_shard_id;
}

template<typename DataRecord>
size_t FullTextShard<DataRecord>::disk_size() const {
	return m_keys.size();
}

