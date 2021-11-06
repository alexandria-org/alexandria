/*
 * MIT License
 *
 * Alexandria.org
 *
 * Copyright (c) 2021 Josef Cullhed, <info@alexandria.org>, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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
#include "FullTextResultSet.h"

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

	FullTextShard(const string &db_name, size_t shard_id, size_t partition);
	~FullTextShard();

	void find(uint64_t key, FullTextResultSet<DataRecord> *result_set);
	size_t read_key_pos(ifstream &reader, uint64_t key);
	size_t total_num_results(uint64_t key);
	void read_keys();
	void unread_keys();
	vector<uint64_t> keys();

	string mountpoint() const;
	string filename() const;
	size_t shard_id() const;

	size_t disk_size() const;

private:

	string m_db_name;
	string m_filename;
	size_t m_shard_id;
	size_t m_partition;

	// These variables always represent what is in the file.
	vector<uint64_t> m_keys;
	const size_t m_key_spread = 20;

	bool m_keys_read;
	
	size_t m_num_keys;

	size_t m_data_start;
	size_t m_pos_start;
	size_t m_len_start;
	size_t m_total_start;
	
};

template<typename DataRecord>
FullTextShard<DataRecord>::FullTextShard(const string &db_name, size_t shard, size_t partition)
: m_shard_id(shard), m_db_name(db_name), m_keys_read(false), m_partition(partition) {
	m_filename = "/mnt/"+mountpoint()+"/full_text/fti_" + m_db_name + "_" + to_string(m_shard_id) + ".idx";
	read_keys();
}

template<typename DataRecord>
FullTextShard<DataRecord>::~FullTextShard() {
}

template<typename DataRecord>
void FullTextShard<DataRecord>::find(uint64_t key, FullTextResultSet<DataRecord> *result_set) {

	ifstream reader(filename(), ios::binary);

	size_t key_pos = read_key_pos(reader, key);

	if (key_pos == SIZE_MAX) {
		result_set->resize(0);
		return;
	}

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

	reader.seekg(m_data_start + pos, ios::beg);

	result_set->prepare_segments(filename(), (size_t)reader.tellg(), len);
	result_set->read_next_segment();

	/*size_t num_records = len / sizeof(DataRecord);
	if (num_records > Config::ft_max_results_per_section) num_records = Config::ft_max_results_per_section;

	result_set->resize(num_records);

	DataRecord *record_res = result_set->data_pointer();

	int fd = open(filename().c_str(), O_RDONLY);
	posix_fadvise(fd, reader.tellg(), num_records * sizeof(DataRecord), POSIX_FADV_SEQUENTIAL);
	lseek(fd, reader.tellg(), SEEK_SET);
	::read(fd, (void *)&record_res[0], (size_t)num_records * sizeof(DataRecord));
	close(fd);*/

	result_set->set_total_num_results(total_num_results);
}

/*
 * Reads the exact position of the key, returns SIZE_MAX if the key was not found.
 * */
template<typename DataRecord>
size_t FullTextShard<DataRecord>::read_key_pos(ifstream &reader, uint64_t key) {

	auto iter = lower_bound(m_keys.begin(), m_keys.end(), key);

	// The last element of m_keys is the actual last key.
	if (iter == m_keys.end()) {
		return SIZE_MAX;
	}

	size_t key_pos = (iter - m_keys.begin()) * m_key_spread;
	if (key_pos >= m_key_spread) key_pos -= m_key_spread;

	uint64_t buffer[21];

	reader.seekg(8 + key_pos * 8, ios::beg);
	reader.read((char *)buffer, (m_num_keys < 21 ? m_num_keys : 21) * 8);

	for (size_t i = 0; i < (m_num_keys < 21 ? m_num_keys : 21); i++) {
		if (buffer[i] == key) return key_pos + i;
	}

	return SIZE_MAX;
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

	vector<uint64_t> temp_keys;
	temp_keys.resize(m_num_keys);

	if (m_num_keys > Config::ft_max_keys) {
		throw LOG_ERROR_EXCEPTION("Number of keys in file exceeeds maximum: file: " + filename() + " num: " + to_string(m_num_keys));
	}

	uint64_t *key_data = temp_keys.data();

	// Read the keys.
	reader.read((char *)key_data, m_num_keys * sizeof(uint64_t));

	m_pos_start = reader.tellg();

	m_len_start = m_pos_start + m_num_keys * 8;
	m_total_start = m_len_start + m_num_keys * 8;
	m_data_start = m_total_start + m_num_keys * 8;

	// Throw away keys.
	if (temp_keys.size() > 0) {
		for (size_t i = 0; i < temp_keys.size(); i++) {
			if (i % m_key_spread == 0) {
				m_keys.push_back(temp_keys[i]);
			}
		}
		if (m_keys.back() != temp_keys.back()) m_keys.push_back(temp_keys.back());
	}
}

template<typename DataRecord>
void FullTextShard<DataRecord>::unread_keys() {
	m_keys_read = false;
	m_keys.clear();
	m_num_keys = 0;
}

template<typename DataRecord>
vector<uint64_t> FullTextShard<DataRecord>::keys() {
	if (!m_keys_read) read_keys();
	return m_keys;
}

template<typename DataRecord>
string FullTextShard<DataRecord>::mountpoint() const {
	if (m_partition < 1) return to_string(m_shard_id % 4);
	return to_string((m_shard_id % 4) + 4);
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

