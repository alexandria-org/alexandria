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

#include <sstream>
#include "config.h"
#include "hash_table_shard_builder.h"
#include "logger/logger.h"
#include "file/file.h"
#include "indexer/merger.h"

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

using namespace std;

namespace hash_table {

	hash_table_shard_builder::hash_table_shard_builder(const string &db_name, size_t shard_id)
	: m_db_name(db_name), m_shard_id(shard_id), m_cache_limit(25 + rand() % 10)
	{
		indexer::merger::register_appender((size_t)this, [this]() {write();}, [this]() { return cache_size(); });
	}

	hash_table_shard_builder::~hash_table_shard_builder() {
		indexer::merger::deregister_merger((size_t)this);
	}

	bool hash_table_shard_builder::full() const {
		return m_cache.size() > m_cache_limit;
	}

	void hash_table_shard_builder::write() {

		std::lock_guard guard(m_lock);

		ofstream outfile(filename_data(), ios::binary | ios::app);
		ofstream outfile_pos(filename_pos(), ios::binary | ios::app);

		size_t last_pos = outfile.tellp();

		for (const auto &iter : m_cache) {
			const size_t version = m_version[iter.first];
			outfile.write((char *)&iter.first, config::ht_key_size);
			outfile.write((char *)&version, sizeof(size_t));

			// Compress data
			stringstream ss(iter.second);

			boost::iostreams::filtering_istream compress_stream;
			compress_stream.push(boost::iostreams::gzip_compressor());
			compress_stream.push(ss);

			stringstream compressed;
			compressed << compress_stream.rdbuf();

			string compressed_string(compressed.str());

			const size_t data_len = compressed_string.size();
			outfile.write((char *)&data_len, sizeof(size_t));

			outfile.write(compressed_string.c_str(), data_len);

			outfile_pos.write((char *)&iter.first, config::ht_key_size);
			outfile_pos.write((char *)&last_pos, sizeof(size_t));
			last_pos += data_len + config::ht_key_size + 2 * sizeof(size_t);
		}

		m_cache = std::map<uint64_t, std::string>{}; // to free memory.
		m_version = std::map<uint64_t, size_t>{};
		m_data_size = 0;
	}

	void hash_table_shard_builder::truncate() {
		std::lock_guard guard(m_lock);
		ofstream outfile(filename_data(), ios::binary | ios::trunc);
		ofstream outfile_pos(filename_pos(), ios::binary | ios::trunc);
	}

	void hash_table_shard_builder::sort() {

		read_keys();

		ofstream outfile_pos(filename_pos(), ios::binary | ios::trunc);
		for (const auto &iter : m_sort_pos) {
			outfile_pos.write((char *)&iter.first, config::ht_key_size);
			outfile_pos.write((char *)&iter.second, sizeof(size_t));
		}
		outfile_pos.close();
		m_sort_pos.clear();
	}

	void hash_table_shard_builder::optimize() {

		std::lock_guard guard(m_lock);

		ifstream infile(filename_data(), ios::binary);

		if (!infile.is_open()) return;

		const size_t buffer_len = 1024*1024*20;
		
		ofstream outfile_data(filename_data_tmp(), ios::binary | ios::trunc);
		ofstream outfile_pos(filename_pos_tmp(), ios::binary | ios::trunc);

		map<uint64_t, string> hash_map;
		map<uint64_t, size_t> versions;
		while (!infile.eof()) {
			uint64_t key;
			if (!infile.read((char *)&key, config::ht_key_size)) break;

			size_t version;
			if (!infile.read((char *)&version, sizeof(size_t))) break;

			size_t data_len;
			if (!infile.read((char *)&data_len, sizeof(size_t))) break;

			std::string data;

			if (data_len > buffer_len) {
				LOG_INFO("data_len " + to_string(data_len) + "is larger than buffer_len " + to_string(buffer_len) + " in file " + filename_data());
				infile.seekg(data_len, ios::cur);
				continue;
			} else {
				data.reserve(data_len);
				std::copy_n(std::istream_iterator<char>(infile), data_len, std::back_inserter(data));
			}

			auto ver_iter = versions.find(key);
			if (ver_iter != versions.end() && ver_iter->second > version) {

			} else {
				hash_map[key] = std::move(data);
				versions[key] = version;
			}
		}

		size_t last_pos = 0;
		for (const auto &iter : hash_map) {
			const uint64_t key = iter.first;
			const size_t version = versions[key];
			const size_t data_len = iter.second.size();
			outfile_data.write((char *)&key, config::ht_key_size);
			outfile_data.write((char *)&version, config::ht_key_size);
			outfile_data.write((char *)&data_len, sizeof(size_t));
			outfile_data.write(iter.second.c_str(), data_len);

			outfile_pos.write((char *)&key, config::ht_key_size);
			outfile_pos.write((char *)&last_pos, sizeof(size_t));

			last_pos += data_len + config::ht_key_size + 2 * sizeof(size_t);
		}

		outfile_data.close();
		outfile_pos.close();

		file::copy_file(filename_data_tmp(), filename_data());
		file::copy_file(filename_pos_tmp(), filename_pos());
		file::delete_file(filename_data_tmp());
		file::delete_file(filename_pos_tmp());

		sort();
	}

	void hash_table_shard_builder::add(uint64_t key, const string &value) {
		std::lock_guard guard(m_lock);
		m_data_size += value.capacity();
		m_cache[key] = value;
		m_version[key] = 0ull;
	}

	void hash_table_shard_builder::add_versioned(uint64_t key, const string &value, size_t version) {
		std::lock_guard guard(m_lock);
		m_data_size += value.capacity();
		auto ver_iter = m_version.find(key);
		if (ver_iter != m_version.end() && ver_iter->second > version) {
			// do nothing
		} else {
			m_cache[key] = value;
			m_version[key] = version;
		}
	}

	size_t hash_table_shard_builder::cache_size() const {
		// This is an OK approximation since m_data_size will be much larger than the keys.
		return m_cache.size() * sizeof(uint64_t) * 2 + m_data_size;
	}

	string hash_table_shard_builder::filename_data() const {
		size_t disk_shard = m_shard_id % 8;
		return "/mnt/" + to_string(disk_shard) + "/hash_table/ht_" + m_db_name + "_" + to_string(m_shard_id) + ".data";
	}

	string hash_table_shard_builder::filename_pos() const {
		size_t disk_shard = m_shard_id % 8;
		return "/mnt/" + to_string(disk_shard) + "/hash_table/ht_" + m_db_name + "_" + to_string(m_shard_id) + ".pos";
	}

	string hash_table_shard_builder::filename_data_tmp() const {
		size_t disk_shard = m_shard_id % 8;
		return "/mnt/" + to_string(disk_shard) + "/hash_table/ht_" + m_db_name + "_" + to_string(m_shard_id) + ".data.tmp";
	}

	string hash_table_shard_builder::filename_pos_tmp() const {
		size_t disk_shard = m_shard_id % 8;
		return "/mnt/" + to_string(disk_shard) + "/hash_table/ht_" + m_db_name + "_" + to_string(m_shard_id) + ".pos.tmp";
	}

	void hash_table_shard_builder::read_keys() {
		ifstream infile(filename_pos(), ios::binary);
		const size_t record_len = config::ht_key_size + sizeof(size_t);
		const size_t buffer_len = record_len * 10000;
		char buffer[buffer_len];

		if (infile.is_open()) {
			do {
				infile.read(buffer, buffer_len);

				size_t read_bytes = infile.gcount();

				for (size_t i = 0; i < read_bytes; i += record_len) {
					const uint64_t key = *((uint64_t *)&buffer[i]);
					const size_t pos = *((size_t *)&buffer[i + config::ht_key_size]);
					m_sort_pos[key] = pos;
				}

			} while (!infile.eof());
		}
		infile.close();
	}

}
