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

#include "config.h"
#include "HashTableShard.h"
#include "system/Logger.h"

HashTableShard::HashTableShard(const string &db_name, size_t shard_id)
: m_db_name(db_name), m_shard_id(shard_id), m_loaded(false), m_size(0)
{
	load();
}

HashTableShard::~HashTableShard() {

}

string HashTableShard::find(uint64_t key) {

	if (!m_loaded) load();

	const uint64_t key_significant = key >> (64-m_significant);
	auto iter = m_pos.find(key_significant);
	if (iter == m_pos.end()) return "";

	auto pos_pair = iter->second;
	size_t pos_in_posfile = pos_pair.first;
	size_t len_in_posfile = pos_pair.second;

	ifstream infile_pos(filename_pos(), ios::binary);
	infile_pos.seekg(pos_in_posfile, ios::beg);

	const size_t record_len = Config::ht_key_size + sizeof(size_t);
	const size_t byte_len = len_in_posfile * record_len;
	const size_t pos_buffer_len = 200000;
	char pos_buffer[pos_buffer_len];
	if (byte_len > pos_buffer_len) {
		throw error("byte_len ("+to_string(byte_len)+") larger than pos_buffer_len ("+to_string(pos_buffer_len)+")");
	}

	infile_pos.read(pos_buffer, byte_len);
	infile_pos.close();

	size_t pos = string::npos;
	for (size_t i = 0; i < byte_len; i+= record_len) {
		uint64_t tmp_key = *((uint64_t *)&pos_buffer[i]);
		if (tmp_key == key) {
			pos = *((size_t *)&pos_buffer[i + Config::ht_key_size]);
		}
	}
	if (pos == string::npos) return "";

	return data_at_position(pos);
}

string HashTableShard::filename_data() const {
	size_t disk_shard = m_shard_id % 8;
	return "/mnt/" + to_string(disk_shard) + "/hash_table/ht_" + m_db_name + "_" + to_string(m_shard_id) + ".data";
}

string HashTableShard::filename_pos() const {
	size_t disk_shard = m_shard_id % 8;
	return "/mnt/" + to_string(disk_shard) + "/hash_table/ht_" + m_db_name + "_" + to_string(m_shard_id) + ".pos";
}

size_t HashTableShard::shard_id() const {
	return m_shard_id;
}

size_t HashTableShard::size() const {
	return m_size;
}

size_t HashTableShard::file_size() const {
	ifstream infile(filename_data(), ios::ate | ios::binary);
	size_t file_size = infile.tellg();
	return file_size;
}

void HashTableShard::load() {
	m_loaded = true;
	ifstream infile(filename_pos(), ios::binary);
	const size_t record_len = Config::ht_key_size + sizeof(size_t);
	const size_t buffer_len = record_len * 10000;
	char buffer[buffer_len];
	size_t latest_pos = 0;

	vector<uint64_t> keys;
	vector<size_t> positions;
	if (infile.is_open()) {
		do {
			infile.read(buffer, buffer_len);

			size_t read_bytes = infile.gcount();

			for (size_t i = 0; i < read_bytes; i += record_len) {
				keys.push_back(*((uint64_t *)&buffer[i]));
				positions.push_back(latest_pos);
				latest_pos += record_len;
			}

		} while (!infile.eof());
	}
	infile.close();

	m_size = keys.size();

	size_t idx = 0;
	for (uint64_t key : keys) {
		const uint64_t key_significant = key >> (64-m_significant);
		if (m_pos.find(key_significant) == m_pos.end()) {
			m_pos[key_significant] = make_pair(positions[idx], 0);
		}
		m_pos[key_significant].second++;
		idx++;
	}

	//LOG_INFO("Loaded shard " + to_string(m_shard_id));
}

void HashTableShard::print_all_items() {

	ifstream infile(filename_pos(), ios::binary);
	const size_t record_len = Config::ht_key_size + sizeof(size_t);
	const size_t buffer_len = record_len * 10000;
	char buffer[buffer_len];
	size_t latest_pos = 0;

	vector<uint64_t> keys;
	vector<size_t> positions;
	if (infile.is_open()) {
		do {
			infile.read(buffer, buffer_len);

			size_t read_bytes = infile.gcount();

			for (size_t i = 0; i < read_bytes; i += record_len) {
				keys.push_back(*((uint64_t *)&buffer[i]));
				positions.push_back(*((size_t *)&buffer[i + Config::ht_key_size]));
			}

		} while (!infile.eof());
	}
	infile.close();

	for (size_t i = 0; i < keys.size(); i++) {
		cout << keys[i] << " => " << data_at_position(positions[i]) << endl;
	}
}

string HashTableShard::data_at_position(size_t pos) {

	ifstream infile(filename_data(), ios::binary);
	infile.seekg(pos, ios::beg);

	// Read key
	uint64_t read_key;
	infile.read((char *)&read_key, sizeof(uint64_t));

	// Read data length.
	size_t data_len;
	infile.read((char *)&data_len, sizeof(size_t));

	char *buffer = new char[data_len];

	infile.read(buffer, data_len);
	stringstream ss(string(buffer, data_len));

	filtering_istream decompress_stream;
	decompress_stream.push(gzip_decompressor());
	decompress_stream.push(ss);

	stringstream decompressed;
	decompressed << decompress_stream.rdbuf();

	delete []buffer;

	return decompressed.str();
}

