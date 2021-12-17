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

#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <algorithm>

template<typename DataRecord> class FullTextShardBuilder;

#include "FullTextIndexer.h"
#include "FullTextIndex.h"
#include "parser/URL.h"
#include "FullTextRecord.h"
#include "UrlToDomain.h"
#include "link_index/LinkFullTextRecord.h"
#include "system/Logger.h"

using namespace std;

template<typename DataRecord>
class FullTextShardBuilder {

public:

	FullTextShardBuilder(const string &db_name, size_t shard_id, size_t partition);
	FullTextShardBuilder(const string &db_name, size_t shard_id, size_t partition, size_t bytes_per_shard);
	~FullTextShardBuilder();

	void add(uint64_t key, const DataRecord &record);
	void sort_cache();
	bool full() const;
	void append();
	void merge();
	bool should_merge();
	void merge_with(FullTextShardBuilder<DataRecord> &with);

	string mountpoint() const;
	string cache_filename() const;
	string key_cache_filename() const;
	string key_filename() const;
	string target_filename() const;

	void truncate();
	void truncate_cache_files();

	size_t disk_size() const;
	size_t cache_size() const;

private:

	const string m_db_name;
	const size_t m_shard_id;
	const size_t m_max_cache_size;
	const size_t m_partition;

	const size_t m_max_cache_file_size = 300 * 1000 * 1000; // 200mb.
	const size_t m_max_num_keys = 10000;
	const size_t m_buffer_len = m_max_num_keys * sizeof(DataRecord); // 1m elements
	char *m_buffer;

	vector<uint64_t> m_keys;
	vector<DataRecord> m_records;

	map<uint64_t, vector<DataRecord>> m_cache;
	map<uint64_t, size_t> m_total_results;

	void read_append_cache();
	void read_data_to_cache();
	bool read_page(ifstream &reader);
	void save_file();
	void write_key(ofstream &key_writer, uint64_t key, size_t page_pos);
	size_t write_page(ofstream &writer, const vector<uint64_t> &keys);
	void reset_key_file(ofstream &key_writer);
	void order_sections_by_value(vector<DataRecord> &results) const;

};

template<typename DataRecord>
FullTextShardBuilder<DataRecord>::FullTextShardBuilder(const string &db_name, size_t shard_id, size_t partition)
: m_db_name(db_name), m_shard_id(shard_id), m_max_cache_size(Config::ft_cached_bytes_per_shard / sizeof(DataRecord)), m_partition(partition) {
}

template<typename DataRecord>
FullTextShardBuilder<DataRecord>::FullTextShardBuilder(const string &db_name, size_t shard_id, size_t partition, size_t bytes_per_shard)
: m_db_name(db_name), m_shard_id(shard_id), m_max_cache_size(bytes_per_shard / sizeof(DataRecord)), m_partition(partition) {
}

template<typename DataRecord>
FullTextShardBuilder<DataRecord>::~FullTextShardBuilder() {
}

template<typename DataRecord>
void FullTextShardBuilder<DataRecord>::add(uint64_t key, const DataRecord &record) {

	// Amortized constant
	m_keys.push_back(key);
	m_records.push_back(record);

}

template<typename DataRecord>
void FullTextShardBuilder<DataRecord>::sort_cache() {
	const size_t max_results_per_partition = Config::ft_max_results_per_section * Config::ft_max_sections;
	for (auto &iter : m_cache) {
		// Make elements unique.
		sort(iter.second.begin(), iter.second.end(), [](const DataRecord &a, const DataRecord &b) {
			return a.m_value < b.m_value;
		});
		auto last = unique(iter.second.begin(), iter.second.end(),
			[](const DataRecord &a, const DataRecord &b) {
			return a.m_value == b.m_value;
		});
		iter.second.erase(last, iter.second.end());

		m_total_results[iter.first] = iter.second.size();

		if (iter.second.size() > Config::ft_max_results_per_section) {
			sort(iter.second.begin(), iter.second.end(), [](const DataRecord &a, const DataRecord &b) {
				return a.m_score > b.m_score;
			});

			// Cap results at the maximum number of results per partition.
			if (iter.second.size() > max_results_per_partition) {
				iter.second.resize(max_results_per_partition);
			}

			// Order each section by value.
			order_sections_by_value(iter.second);
		}
	}
}

template<typename DataRecord>
void FullTextShardBuilder<DataRecord>::order_sections_by_value(vector<DataRecord> &results) const {
	bool stop = false;
	for (size_t section = 0; section < Config::ft_max_sections; section++) {
		const size_t start = section * Config::ft_max_results_per_section;
		size_t end = start + Config::ft_max_results_per_section;
		if (end > results.size()) {
			end = results.size();
			stop = true;
		}
		sort(results.begin() + start, results.begin() + end, [](const DataRecord &a, const DataRecord &b) {
			return a.m_value < b.m_value;
		});
		if (stop) break;
	}
}

template<typename DataRecord>
bool FullTextShardBuilder<DataRecord>::full() const {
	return cache_size() > m_max_cache_size;
}

template<typename DataRecord>
void FullTextShardBuilder<DataRecord>::append() {
	ofstream record_writer(cache_filename(), ios::binary | ios::app);
	if (!record_writer.is_open()) {
		throw LOG_ERROR_EXCEPTION("Could not open full text shard (" + cache_filename() + "). Error: " +
			string(strerror(errno)));
	}

	ofstream key_writer(key_cache_filename(), ios::binary | ios::app);
	if (!key_writer.is_open()) {
		throw LOG_ERROR_EXCEPTION("Could not open full text shard (" + key_cache_filename() + "). Error: " +
			string(strerror(errno)));
	}

	record_writer.write((const char *)m_records.data(), m_records.size() * sizeof(DataRecord));
	key_writer.write((const char *)m_keys.data(), m_keys.size() * sizeof(uint64_t));

	m_records.clear();
	m_keys.clear();
}

template<typename DataRecord>
void FullTextShardBuilder<DataRecord>::merge() {

	read_append_cache();
	sort_cache();
	save_file();
	truncate_cache_files();

}

template<typename DataRecord>
void FullTextShardBuilder<DataRecord>::read_append_cache() {

	m_cache.clear();
	m_total_results.clear();

	// Read the current file.
	read_data_to_cache();

	// Read the cache into memory.
	ifstream reader(cache_filename(), ios::binary);
	if (!reader.is_open()) {
		throw LOG_ERROR_EXCEPTION("Could not open full text shard (" + cache_filename() + "). Error: " + string(strerror(errno)));
	}

	ifstream key_reader(key_cache_filename(), ios::binary);
	if (!key_reader.is_open()) {
		throw LOG_ERROR_EXCEPTION("Could not open full text shard (" + key_cache_filename() + "). Error: " + string(strerror(errno)));
	}

	const size_t buffer_len = 100000;
	const size_t buffer_size = sizeof(DataRecord) * buffer_len;
	const size_t key_buffer_size = sizeof(uint64_t) * buffer_len;
	char *buffer = new char[buffer_size];
	char *key_buffer = new char[key_buffer_size];

	reader.seekg(0, ios::beg);

	while (!reader.eof()) {

		reader.read(buffer, buffer_size);
		key_reader.read(key_buffer, key_buffer_size);

		const size_t read_bytes = reader.gcount();
		const size_t num_records = read_bytes / sizeof(DataRecord);

		for (size_t i = 0; i < num_records; i++) {
			const DataRecord *record = (DataRecord *)&buffer[i * sizeof(DataRecord)];
			const uint64_t key = *((uint64_t *)&key_buffer[i * sizeof(uint64_t)]);
			m_cache[key].push_back(*record);
		}
	}

	delete key_buffer;
	delete buffer;
}

template<typename DataRecord>
bool FullTextShardBuilder<DataRecord>::should_merge() {

	ofstream writer(cache_filename(), ios::binary | ios::app);
	size_t cache_file_size = writer.tellp();

	return cache_file_size > m_max_cache_file_size;
}

template<typename DataRecord>
void FullTextShardBuilder<DataRecord>::merge_with(FullTextShardBuilder<DataRecord> &with) {

	// Read everything to cache.
	read_data_to_cache();
	with.read_data_to_cache();

	// Copy vectors from with that are not present in m_cache.
	for (auto &iter : with.m_cache) {
		if (m_cache.find(iter.first) == m_cache.end()) m_cache[iter.first] = iter.second;
	}

	// Merge algorithm as described here: https://en.wikipedia.org/wiki/Merge_algorithm
	// but with an additional sum of the scores.
	for (auto &iter : m_cache) {
		const vector<DataRecord> *vec1 = &iter.second;
		const vector<DataRecord> *vec2 = &(with.m_cache[iter.first]);
		size_t i = 0;
		size_t j = 0;
		vector<DataRecord> merged;
		while (i < vec1->size() && j < vec2->size()) {
			if (vec1->at(i).m_value < vec2->at(j).m_value) {
				merged.push_back(vec1->at(i));
				i++;
			} else if (vec1->at(i).m_value == vec2->at(j).m_value) {
				// Sum the scores.
				/*merged.push_back(vec1->at(i));
				merged.back().m_score += vec2->at(j).m_score;*/
				i++;
				j++;
			} else {
				merged.push_back(vec2->at(j));
				j++;
			}
		}
		for ( ; i < vec1->size(); i++) merged.push_back(vec1->at(i));
		for ( ; j < vec2->size(); j++) merged.push_back(vec2->at(j));

		m_cache[iter.first] = merged;
	}

	save_file();
	with.truncate();
}

/*
 * Reads the file into RAM.
 * */
template<typename DataRecord>
void FullTextShardBuilder<DataRecord>::read_data_to_cache() {

	m_cache.clear();
	m_total_results.clear();

	ifstream reader(target_filename(), ios::binary);
	if (!reader.is_open()) return;

	reader.seekg(0, ios::end);
	const size_t file_size = reader.tellg();
	if (file_size == 0) return;
	reader.seekg(0, ios::beg);

	m_buffer = new char[m_buffer_len];
	while (read_page(reader)) {
	}
	delete m_buffer;
}

template<typename DataRecord>
bool FullTextShardBuilder<DataRecord>::read_page(ifstream &reader) {

	char buffer[64];

	reader.read(buffer, 8);

	if (reader.eof()) return false;

	uint64_t num_keys = *((uint64_t *)(&buffer[0]));

	char *vector_buffer = new char[num_keys * 8];

	// Read the keys.
	reader.read(vector_buffer, num_keys * 8);
	vector<uint64_t> keys;
	for (size_t i = 0; i < num_keys; i++) {
		keys.push_back(*((uint64_t *)(&vector_buffer[i*8])));
	}

	// Read the positions.
	reader.read(vector_buffer, num_keys * 8);
	vector<size_t> positions;
	for (size_t i = 0; i < num_keys; i++) {
		positions.push_back(*((size_t *)(&vector_buffer[i*8])));
	}

	// Read the lengths.
	reader.read(vector_buffer, num_keys * 8);
	vector<size_t> lens;
	size_t data_size = 0;
	for (size_t i = 0; i < num_keys; i++) {
		size_t len = *((size_t *)(&vector_buffer[i*8]));
		lens.push_back(len);
		data_size += len;
	}

	// Read the totals.
	reader.read(vector_buffer, num_keys * 8);
	for (size_t i = 0; i < num_keys; i++) {
		size_t total = *((size_t *)(&vector_buffer[i*8]));
		m_total_results[keys[i]] = total;
	}
	delete vector_buffer;

	if (data_size == 0) return true;

	// Read the data.
	size_t total_read_data = 0;
	size_t key_id = 0;
	size_t num_records_for_key = lens[key_id] / sizeof(DataRecord);
	while (total_read_data < data_size) {
		reader.read(m_buffer, min(m_buffer_len, data_size));
		const size_t read_len = reader.gcount();

		if (read_len == 0) {
			LOG_INFO("Data stopped before end. Ignoring shard " + m_shard_id);
			m_cache.clear();
			break;
		}

		total_read_data += read_len;

		size_t num_records = read_len / sizeof(DataRecord);
		for (size_t i = 0; i < num_records; i++) {
			while (num_records_for_key == 0 && key_id < num_keys) {
				key_id++;
				num_records_for_key = lens[key_id] / sizeof(DataRecord);
			}

			if (num_records_for_key > 0) {

				const DataRecord *record = (DataRecord *)&m_buffer[i * sizeof(DataRecord)];
				
				m_cache[keys[key_id]].push_back(*record);

				num_records_for_key--;
			}
		}
	}

	return true;
}

template<typename DataRecord>
void FullTextShardBuilder<DataRecord>::save_file() {

	ofstream writer(target_filename(), ios::binary | ios::trunc);
	if (!writer.is_open()) {
		throw LOG_ERROR_EXCEPTION("Could not open full text shard. Error: " + string(strerror(errno)));
	}

	ofstream key_writer(key_filename(), ios::binary | ios::trunc);
	if (!key_writer.is_open()) {
		throw LOG_ERROR_EXCEPTION("Could not open full text shard. Error: " + string(strerror(errno)));
	}

	reset_key_file(key_writer);

	unordered_map<uint64_t, vector<uint64_t>> pages;
	for (auto &iter : m_cache) {
		pages[iter.first % Config::shard_hash_table_size].push_back(iter.first);
	}

	for (const auto &iter : pages) {
		const size_t page_pos = write_page(writer, iter.second);
		writer.flush();
		write_key(key_writer, iter.first, page_pos);
	}

	/*sort(keys.begin(), keys.end(), [](const uint64_t a, const uint64_t b) {
		return a < b;
	});

	

	m_cache.clear();
	*/
}

template<typename DataRecord>
void FullTextShardBuilder<DataRecord>::write_key(ofstream &key_writer, uint64_t key, size_t page_pos) {
	assert(key < Config::shard_hash_table_size);
	key_writer.seekp(key * sizeof(uint64_t));
	key_writer.write((char *)&page_pos, sizeof(size_t));
}

/*
 * Writes the page with keys, appending it to the file stream writer. Takes data from m_cache.
 * */
template<typename DataRecord>
size_t FullTextShardBuilder<DataRecord>::write_page(ofstream &writer, const vector<uint64_t> &keys) {

	const size_t page_pos = writer.tellp();

	size_t num_keys = keys.size();

	writer.write((char *)&num_keys, 8);
	writer.write((char *)keys.data(), keys.size() * 8);

	vector<size_t> v_pos;
	vector<size_t> v_len;
	vector<size_t> v_tot;

	size_t pos = 0;
	for (uint64_t key : keys) {

		// Store position and length
		size_t len = m_cache[key].size() * sizeof(DataRecord);
		
		v_pos.push_back(pos);
		v_len.push_back(len);
		v_tot.push_back(m_total_results[key]);

		pos += len;
	}
	
	writer.write((char *)v_pos.data(), keys.size() * 8);
	writer.write((char *)v_len.data(), keys.size() * 8);
	writer.write((char *)v_tot.data(), keys.size() * 8);

	// Write data.
	for (uint64_t key : keys) {
		writer.write((char *)m_cache[key].data(), sizeof(DataRecord) * m_cache[key].size());
	}

	return page_pos;
}

template<typename DataRecord>
void FullTextShardBuilder<DataRecord>::reset_key_file(ofstream &key_writer) {
	key_writer.seekp(0);
	uint64_t data = SIZE_MAX;
	for (size_t i = 0; i < Config::shard_hash_table_size; i++) {
		key_writer.write((char *)&data, sizeof(uint64_t));
	}
}

template<typename DataRecord>
string FullTextShardBuilder<DataRecord>::mountpoint() const {
	if (m_partition < 1) return to_string(m_shard_id % 4);
	return to_string((m_shard_id % 4) + 4);
}

template<typename DataRecord>
string FullTextShardBuilder<DataRecord>::cache_filename() const {
	return "/mnt/" + mountpoint() + "/output/precache_" + m_db_name + "_" + to_string(m_shard_id) + ".cache";
}

template<typename DataRecord>
string FullTextShardBuilder<DataRecord>::key_cache_filename() const {
	return "/mnt/" + mountpoint() + "/output/precache_" + m_db_name + "_" + to_string(m_shard_id) +".keys";
}

template<typename DataRecord>
string FullTextShardBuilder<DataRecord>::key_filename() const {
	return "/mnt/" + mountpoint() + "/full_text/fti_" + m_db_name + "_" + to_string(m_shard_id) + ".keys";
}

template<typename DataRecord>
string FullTextShardBuilder<DataRecord>::target_filename() const {
	return "/mnt/" + mountpoint() + "/full_text/fti_" + m_db_name + "_" + to_string(m_shard_id) + ".idx";
}

/*
	Deletes ALL data from this shard.
*/
template<typename DataRecord>
void FullTextShardBuilder<DataRecord>::truncate() {

	truncate_cache_files();

	ofstream target_writer(target_filename(), ios::trunc);
	target_writer.close();
}

/*
	Deletes all data from caches.
*/
template<typename DataRecord>
void FullTextShardBuilder<DataRecord>::truncate_cache_files() {

	m_cache.clear();

	ofstream writer(cache_filename(), ios::trunc);
	writer.close();

	ofstream key_writer(key_cache_filename(), ios::trunc);
	key_writer.close();
}

template<typename DataRecord>
size_t FullTextShardBuilder<DataRecord>::disk_size() const {

	ifstream reader(cache_filename(), ios::binary);
	if (!reader.is_open()) {
		throw LOG_ERROR_EXCEPTION("Could not open full text shard (" + cache_filename() + "). Error: " + string(strerror(errno)));
	}

	reader.seekg(0, ios::end);
	size_t file_size = reader.tellg();

	return file_size;
}

template<typename DataRecord>
size_t FullTextShardBuilder<DataRecord>::cache_size() const {
	return m_records.size();
}

