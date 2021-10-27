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
#include "HashTableShardBuilder.h"
#include "system/Logger.h"
#include "file/File.h"

HashTableShardBuilder::HashTableShardBuilder(const string &db_name, size_t shard_id)
: m_db_name(db_name), m_shard_id(shard_id), m_cache_limit(25 + rand() % 10)
{

}

HashTableShardBuilder::~HashTableShardBuilder() {

}

bool HashTableShardBuilder::full() const {
	return m_cache.size() > m_cache_limit;
}

void HashTableShardBuilder::write() {
	ofstream outfile(filename_data(), ios::binary | ios::app);
	ofstream outfile_pos(filename_pos(), ios::binary | ios::app);

	size_t last_pos = outfile.tellp();

	for (const auto &iter : m_cache) {
		outfile.write((char *)&iter.first, Config::ht_key_size);

		// Compress data
		stringstream ss(iter.second);

		filtering_istream compress_stream;
		compress_stream.push(gzip_compressor());
		compress_stream.push(ss);

		stringstream compressed;
		compressed << compress_stream.rdbuf();

		string compressed_string(compressed.str());

		const size_t data_len = compressed_string.size();
		outfile.write((char *)&data_len, sizeof(size_t));

		outfile.write(compressed_string.c_str(), data_len);

		outfile_pos.write((char *)&iter.first, Config::ht_key_size);
		outfile_pos.write((char *)&last_pos, sizeof(size_t));
		last_pos += data_len + Config::ht_key_size + sizeof(size_t);
	}

	m_cache.clear();
}

void HashTableShardBuilder::truncate() {
	ofstream outfile(filename_data(), ios::binary | ios::trunc);
	ofstream outfile_pos(filename_pos(), ios::binary | ios::trunc);
}

void HashTableShardBuilder::sort() {

	read_keys();

	ofstream outfile_pos(filename_pos(), ios::binary | ios::trunc);
	for (const auto &iter : m_sort_pos) {
		outfile_pos.write((char *)&iter.first, Config::ht_key_size);
		outfile_pos.write((char *)&iter.second, sizeof(size_t));
	}
	outfile_pos.close();
	m_sort_pos.clear();
}

void HashTableShardBuilder::optimize() {

	ifstream infile(filename_data(), ios::binary);

	const size_t buffer_len = 1024*1024*10;
	char *buffer = new char[buffer_len];

	ofstream outfile_data(filename_data_tmp(), ios::binary | ios::trunc);
	ofstream outfile_pos(filename_pos_tmp(), ios::binary | ios::trunc);

	map<size_t, string> hash_map;
	while (!infile.eof()) {
		size_t key;
		infile.read((char *)&key, Config::ht_key_size);

		size_t data_len;
		infile.read((char *)&data_len, sizeof(size_t));

		if (data_len > buffer_len) {
			LOG_INFO("len is larger than buffer_len");
			infile.seekg(data_len, ios::cur);
			continue;
		} else {
			infile.read(buffer, data_len);
		}

		hash_map[key] = string(buffer, data_len);
	}

	size_t last_pos = 0;
	for (const auto &iter : hash_map) {
		const size_t key = iter.first;
		const size_t data_len = iter.second.size();
		outfile_data.write((char *)&key, Config::ht_key_size);
		outfile_data.write((char *)&data_len, sizeof(size_t));
		outfile_data.write(iter.second.c_str(), data_len);

		outfile_pos.write((char *)&key, Config::ht_key_size);
		outfile_pos.write((char *)&last_pos, sizeof(size_t));

		last_pos += data_len + Config::ht_key_size + sizeof(size_t);
	}

	outfile_data.close();
	outfile_pos.close();

	File::copy_file(filename_data_tmp(), filename_data());
	File::copy_file(filename_pos_tmp(), filename_pos());
	File::delete_file(filename_data_tmp());
	File::delete_file(filename_pos_tmp());

	sort();
}

void HashTableShardBuilder::add(uint64_t key, const string &value) {
	m_cache[key] = value;
}

string HashTableShardBuilder::filename_data() const {
	size_t disk_shard = m_shard_id % 8;
	return "/mnt/" + to_string(disk_shard) + "/hash_table/ht_" + m_db_name + "_" + to_string(m_shard_id) + ".data";
}

string HashTableShardBuilder::filename_pos() const {
	size_t disk_shard = m_shard_id % 8;
	return "/mnt/" + to_string(disk_shard) + "/hash_table/ht_" + m_db_name + "_" + to_string(m_shard_id) + ".pos";
}

string HashTableShardBuilder::filename_data_tmp() const {
	size_t disk_shard = m_shard_id % 8;
	return "/mnt/" + to_string(disk_shard) + "/hash_table/ht_" + m_db_name + "_" + to_string(m_shard_id) + ".data.tmp";
}

string HashTableShardBuilder::filename_pos_tmp() const {
	size_t disk_shard = m_shard_id % 8;
	return "/mnt/" + to_string(disk_shard) + "/hash_table/ht_" + m_db_name + "_" + to_string(m_shard_id) + ".pos.tmp";
}

void HashTableShardBuilder::read_keys() {
	ifstream infile(filename_pos(), ios::binary);
	const size_t record_len = Config::ht_key_size + sizeof(size_t);
	const size_t buffer_len = record_len * 10000;
	char buffer[buffer_len];
	size_t latest_pos = 0;

	if (infile.is_open()) {
		do {
			infile.read(buffer, buffer_len);

			size_t read_bytes = infile.gcount();

			for (size_t i = 0; i < read_bytes; i += record_len) {
				const uint64_t key = *((uint64_t *)&buffer[i]);
				const size_t pos = *((size_t *)&buffer[i + Config::ht_key_size]);
				m_sort_pos[key] = pos;
			}

		} while (!infile.eof());
	}
	infile.close();
}

