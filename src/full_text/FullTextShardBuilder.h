
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
#include "FullTextResult.h"
#include "parser/URL.h"
#include "FullTextRecord.h"
#include "UrlToDomain.h"
#include "link_index/LinkFullTextRecord.h"
#include "system/Logger.h"

using namespace std;

template<typename DataRecord>
class FullTextShardBuilder {

public:

	FullTextShardBuilder(const string &db_name, size_t shard_id);
	FullTextShardBuilder(const string &db_name, size_t shard_id, size_t bytes_per_shard);
	~FullTextShardBuilder();

	void add(uint64_t key, const DataRecord &record);
	void sort_cache();
	void sort_cache_with_sum();
	bool full() const;
	void append();
	void merge();
	void merge_with_sum();
	void read_append_cache();
	bool should_merge();
	void merge_with(FullTextShardBuilder<DataRecord> &with);
	void merge_domain(FullTextShardBuilder<DataRecord> &urls, FullTextShardBuilder<DataRecord> &domains,
		const UrlToDomain *url_to_domain);

	string mountpoint() const;
	string filename() const;
	string key_filename() const;
	string target_filename() const;

	void truncate();
	void truncate_cache_files();

	size_t disk_size() const;
	size_t cache_size() const;

	void read_data_to_cache();

private:

	const string m_db_name;
	const size_t m_shard_id;

	mutable ifstream m_reader;
	ofstream m_writer;
	const size_t m_max_results = 4000000;

	const size_t m_max_cache_file_size = 300 * 1000 * 1000; // 200mb.
	const size_t m_max_cache_size;
	const size_t m_max_num_keys = 10000000;
	const size_t m_buffer_len = m_max_num_keys * sizeof(DataRecord); // 1m elements
	char *m_buffer;

	vector<uint64_t *> m_keys;
	vector<DataRecord *>m_records;
	size_t m_records_position;

	map<uint64_t, vector<DataRecord>> m_cache;
	map<uint64_t, size_t> m_total_results;

	void save_file();

};

template<typename DataRecord>
FullTextShardBuilder<DataRecord>::FullTextShardBuilder(const string &db_name, size_t shard_id)
: m_db_name(db_name), m_shard_id(shard_id), m_max_cache_size(FT_INDEXER_CACHE_BYTES_PER_SHARD / sizeof(DataRecord)) {
	m_records.push_back(new DataRecord[m_max_cache_size]);
	m_keys.push_back(new uint64_t[m_max_cache_size]);
	m_records_position = 0;
}

template<typename DataRecord>
FullTextShardBuilder<DataRecord>::FullTextShardBuilder(const string &db_name, size_t shard_id, size_t bytes_per_shard)
: m_db_name(db_name), m_shard_id(shard_id), m_max_cache_size(bytes_per_shard / sizeof(DataRecord)) {
	m_records.push_back(new DataRecord[m_max_cache_size]);
	m_keys.push_back(new uint64_t[m_max_cache_size]);
	m_records_position = 0;
}

template<typename DataRecord>
FullTextShardBuilder<DataRecord>::~FullTextShardBuilder() {
	if (m_reader.is_open()) {
		m_reader.close();
	}
	for (DataRecord *inputs : m_records) {
		delete inputs;
	}
	for (uint64_t *keys : m_keys) {
		delete keys;
	}
}

template<typename DataRecord>
void FullTextShardBuilder<DataRecord>::add(uint64_t key, const DataRecord &record) {

	uint64_t *keys = m_keys.back();
	DataRecord *records = m_records.back();

	keys[m_records_position] = key;
	records[m_records_position] = record;

	m_records_position++;

	if (m_records_position >= m_max_cache_size) {
		m_records.push_back(new DataRecord[m_max_cache_size]);
		m_keys.push_back(new uint64_t[m_max_cache_size]);
		m_records_position = 0;
	}
}

template<typename DataRecord>
void FullTextShardBuilder<DataRecord>::sort_cache() {
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

		// Cap at m_max_results
		if (iter.second.size() > m_max_results) {
			sort(iter.second.begin(), iter.second.end(), [](const DataRecord &a, const DataRecord &b) {
				return a.m_score > b.m_score;
			});
			iter.second.resize(m_max_results);

			// Order by value again.
			sort(iter.second.begin(), iter.second.end(), [](const DataRecord &a, const DataRecord &b) {
				return a.m_value < b.m_value;
			});
		}

	}
}

template<typename DataRecord>
void FullTextShardBuilder<DataRecord>::sort_cache_with_sum() {
	for (auto &iter : m_cache) {
		// Make elements unique.
		sort(iter.second.begin(), iter.second.end(), [](const DataRecord &a, const DataRecord &b) {
			return a.m_value < b.m_value;
		});

		size_t sum_pos = 0;
		for (size_t i = 1; i < iter.second.size(); i++) {
			if (iter.second[sum_pos].m_value == iter.second[i].m_value) {
				iter.second[sum_pos].m_score += iter.second[i].m_score;
			} else {
				sum_pos++;
				iter.second[sum_pos] = iter.second[i];
			}
		}
		sum_pos++;

		iter.second.erase(iter.second.begin() + sum_pos, iter.second.end());

		m_total_results[iter.first] = iter.second.size();

		// Cap at m_max_results
		if (iter.second.size() > m_max_results) {
			sort(iter.second.begin(), iter.second.end(), [](const DataRecord &a, const DataRecord &b) {
				return a.m_score > b.m_score;
			});
			iter.second.resize(m_max_results);

			// Order by value again.
			sort(iter.second.begin(), iter.second.end(), [](const DataRecord &a, const DataRecord &b) {
				return a.m_value < b.m_value;
			});
		}

	}
}

template<typename DataRecord>
bool FullTextShardBuilder<DataRecord>::full() const {
	return cache_size() > m_max_cache_size;
}

template<typename DataRecord>
void FullTextShardBuilder<DataRecord>::append() {
	m_writer.open(filename(), ios::binary | ios::app);
	if (!m_writer.is_open()) {
		throw error("Could not open full text shard (" + filename() + "). Error: " +
			string(strerror(errno)));
	}

	ofstream key_writer(key_filename(), ios::binary | ios::app);
	if (!key_writer.is_open()) {
		throw error("Could not open full text shard (" + key_filename() + "). Error: " +
			string(strerror(errno)));
	}

	while (m_records.size() > 1) {
		DataRecord *records = m_records.back();
		uint64_t *keys = m_keys.back();
		m_records.pop_back();
		m_keys.pop_back();
		m_writer.write((const char *)records, m_records_position * sizeof(DataRecord));
		key_writer.write((const char *)keys, m_records_position * sizeof(uint64_t));
		m_records_position = m_max_cache_size;
		delete records;
		delete keys;
	}
	m_writer.write((const char *)m_records[0], m_records_position * sizeof(DataRecord));
	key_writer.write((const char *)m_keys[0], m_records_position * sizeof(uint64_t));
	m_records_position = 0;

	m_writer.close();
	key_writer.close();
}

template<typename DataRecord>
void FullTextShardBuilder<DataRecord>::merge() {

	read_append_cache();

	sort_cache();

	save_file();

	truncate_cache_files();
}

template<typename DataRecord>
void FullTextShardBuilder<DataRecord>::merge_with_sum() {
	read_append_cache();

	sort_cache_with_sum();

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
	m_reader.open(filename(), ios::binary);
	if (!m_reader.is_open()) {
		throw error("Could not open full text shard (" + filename() + "). Error: " + string(strerror(errno)));
	}

	ifstream key_reader(key_filename(), ios::binary);
	if (!key_reader.is_open()) {
		throw error("Could not open full text shard (" + key_filename() + "). Error: " + string(strerror(errno)));
	}

	const size_t buffer_len = 100000;
	const size_t buffer_size = sizeof(DataRecord) * buffer_len;
	const size_t key_buffer_size = sizeof(uint64_t) * buffer_len;
	char *buffer = new char[buffer_size];
	char *key_buffer = new char[key_buffer_size];

	m_reader.seekg(0, ios::beg);

	while (!m_reader.eof()) {

		m_reader.read(buffer, buffer_size);
		key_reader.read(key_buffer, key_buffer_size);

		const size_t read_bytes = m_reader.gcount();
		const size_t num_records = read_bytes / sizeof(DataRecord);

		for (size_t i = 0; i < num_records; i++) {
			const DataRecord *record = (DataRecord *)&buffer[i * sizeof(DataRecord)];
			const uint64_t key = *((uint64_t *)&key_buffer[i * sizeof(uint64_t)]);
			m_cache[key].push_back(*record);
		}
	}
	m_reader.close();
	key_reader.close();

	delete key_buffer;
	delete buffer;
}

template<typename DataRecord>
bool FullTextShardBuilder<DataRecord>::should_merge() {

	m_writer.open(filename(), ios::binary | ios::app);
	size_t cache_file_size = m_writer.tellp();
	m_writer.close();

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

template<typename DataRecord>
void FullTextShardBuilder<DataRecord>::merge_domain(FullTextShardBuilder<DataRecord> &urls,
	FullTextShardBuilder<DataRecord> &domains, const UrlToDomain *url_to_domain) {

	// Read everything to cache.
	read_data_to_cache();
	urls.read_data_to_cache();
	domains.read_data_to_cache();

	// Copy vectors from urls that are not present in m_cache.
	for (auto &iter : urls.m_cache) {
		if (m_cache.find(iter.first) == m_cache.end()) m_cache[iter.first] = iter.second;
	}

	// Merge algorithm as described here: https://en.wikipedia.org/wiki/Merge_algorithm
	// but with an additional sum of the scores.
	float total_added_url = 0.0;
	size_t num_added_url = 0;
	float total_added_domain = 0.0;
	size_t num_added_domain = 0;
	for (auto &iter : m_cache) {
		const vector<DataRecord> *vec1 = &iter.second;
		const vector<DataRecord> *vec2 = &(urls.m_cache[iter.first]);
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
				merged.back().m_score += vec2->at(j).m_score;

				total_added_url += vec2->at(j).m_score;
				num_added_url++;*/

				i++;
				j++;
			} else {
				merged.push_back(vec2->at(j));
				j++;
			}
		}
		for ( ; i < vec1->size(); i++) merged.push_back(vec1->at(i));
		for ( ; j < vec2->size(); j++) merged.push_back(vec2->at(j));

		/*
		// Update scores according to domains in the sorted vector merged
		const vector<DataRecord> *vec3 = &(domains.m_cache[iter.first]);
		const size_t host_bits = 20;
		i = 0;
		j = 0;
		while (i < merged.size() && j < vec3->size()) {
			const uint64_t host_part1 = (merged.at(i).m_value >> (64 - host_bits)) << (64 - host_bits);
			const uint64_t host_part2 = (vec3->at(j).m_value >> (64 - host_bits)) << (64 - host_bits);
			if (host_part1 < host_part2) {
				i++;
			} else if (host_part1 == host_part2) {
				// Actually check if the hosts are the same
				auto iter = url_to_domain->url_to_domain().find(merged.at(i).m_value);
				if (iter != url_to_domain->url_to_domain().end() && iter->second == vec3->at(j).m_value) {
					// Add score.
					merged[i].m_score += vec3->at(j).m_score;
					total_added_domain += vec3->at(j).m_score;
					num_added_domain++;
				}
				i++;
			} else {
				// host_part1 > host_part2
				j++;
			}
		}*/

		m_cache[iter.first] = merged;
	}

	cout << "DEBUG, added_score_url: " << total_added_url << " (" << num_added_url << ") added_score_domain: " << total_added_domain << " (" << num_added_domain << ")" << endl;

	save_file();
	urls.truncate();
	domains.truncate();
}

template<typename DataRecord>
void FullTextShardBuilder<DataRecord>::read_data_to_cache() {

	m_cache.clear();
	m_total_results.clear();

	ifstream reader(target_filename(), ios::binary);

	if (!reader.is_open()) return;

	reader.seekg(0, ios::end);
	const size_t file_size = reader.tellg();

	if (file_size == 0) return;

	char buffer[64];

	reader.seekg(0, ios::beg);
	reader.read(buffer, 8);

	uint64_t num_keys = *((uint64_t *)(&buffer[0]));

	if (num_keys > FULL_TEXT_MAX_KEYS) {
		throw error("Number of keys in file exceeeds maximum: file: " + filename() + " num: " + to_string(num_keys));
	}

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

	if (data_size == 0) return;

	m_buffer = new char[m_buffer_len];

	// Read the data.
	size_t total_read_data = 0;
	size_t key_id = 0;
	size_t num_records_for_key = lens[key_id] / sizeof(DataRecord);
	while (total_read_data < data_size) {
		reader.read(m_buffer, m_buffer_len);
		const size_t read_len = reader.gcount();

		if (read_len == 0) {
			LogInfo("Data stopped before end. Ignoring shard " + m_shard_id);
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

	delete m_buffer;

}

template<typename DataRecord>
void FullTextShardBuilder<DataRecord>::save_file() {

	vector<uint64_t> keys;

	const string filename = target_filename();

	m_writer.open(filename, ios::binary | ios::trunc);
	if (!m_writer.is_open()) {
		throw error("Could not open full text shard. Error: " + string(strerror(errno)));
	}

	keys.clear();
	for (auto &iter : m_cache) {
		keys.push_back(iter.first);
	}
	
	sort(keys.begin(), keys.end(), [](const uint64_t a, const uint64_t b) {
		return a < b;
	});

	size_t num_keys = keys.size();

	m_writer.write((char *)&num_keys, 8);
	m_writer.write((char *)keys.data(), keys.size() * 8);

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
	
	m_writer.write((char *)v_pos.data(), keys.size() * 8);
	m_writer.write((char *)v_len.data(), keys.size() * 8);
	m_writer.write((char *)v_tot.data(), keys.size() * 8);

	const size_t buffer_num_records = 1000;
	const size_t buffer_len =  buffer_num_records * sizeof(DataRecord);
	char buffer[buffer_len];

	// Write data.
	hash<string> hasher;
	for (uint64_t key : keys) {
		size_t i = 0;

		for (const DataRecord &record : m_cache[key]) {
			memcpy(&buffer[i], (char *)&record, sizeof(DataRecord));
			i += sizeof(DataRecord);
			if (i == buffer_len) {
				m_writer.write(buffer, buffer_len);
				i = 0;
			}
		}
		if (i) {
			m_writer.write(buffer, i);
		}
	}

	m_writer.close();
	m_cache.clear();
}

template<typename DataRecord>
string FullTextShardBuilder<DataRecord>::mountpoint() const {
	hash<string> hasher;
	return to_string((hasher(m_db_name) + m_shard_id) % 8);
}

template<typename DataRecord>
string FullTextShardBuilder<DataRecord>::filename() const {
	return "/mnt/" + mountpoint() + "/output/precache_" + m_db_name + "_" + to_string(m_shard_id) +
		".cache";
}

template<typename DataRecord>
string FullTextShardBuilder<DataRecord>::key_filename() const {
	return "/mnt/" + mountpoint() + "/output/precache_" + m_db_name + "_" + to_string(m_shard_id) +
		".keys";
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

	m_writer.open(filename(), ios::trunc);
	m_writer.close();

	ofstream key_writer(key_filename(), ios::trunc);
	key_writer.close();
}

template<typename DataRecord>
size_t FullTextShardBuilder<DataRecord>::disk_size() const {

	m_reader.open(filename(), ios::binary);
	if (!m_reader.is_open()) {
		throw error("Could not open full text shard (" + filename() + "). Error: " + string(strerror(errno)));
	}

	m_reader.seekg(0, ios::end);
	size_t file_size = m_reader.tellg();
	m_reader.close();
	return file_size;
}

template<typename DataRecord>
size_t FullTextShardBuilder<DataRecord>::cache_size() const {
	return m_records_position + (m_records.size() - 1) * m_max_cache_size;
}

