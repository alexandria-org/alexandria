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
#include <vector>
#include <map>
#include <set>
#include <unordered_set>
#include <cstring>
#include <cassert>
#include <boost/filesystem.hpp>
#include "merger.h"
#include "score_builder.h"
#include "algorithm/hyper_log_log.h"
#include "config.h"
#include "profiler/profiler.h"
#include "logger/logger.h"
#include "memory/debugger.h"
#include "file/file.h"
#include "index_base.h"

namespace indexer {

	template<typename data_record>
	class basic_index_builder : public index_base<data_record>{
	private:
		// Non copyable
		basic_index_builder(const basic_index_builder &);
		basic_index_builder& operator=(const basic_index_builder &);
	public:

		basic_index_builder(const std::string &file_name);
		basic_index_builder(const std::string &db_name, size_t id);
		basic_index_builder(const std::string &db_name, size_t id, size_t hash_table_size);
		basic_index_builder(const std::string &db_name, size_t id, size_t hash_table_size, size_t max_results);
		~basic_index_builder();

		void add(uint64_t key, const data_record &record);
		size_t cache_size() const;
		
		void append();
		void merge();
		void transform(const std::function<data_record(const data_record &, size_t)> &transform);
		void sort_by(const std::function<bool(const data_record &a, const data_record &b)> sort_by);

		void truncate();
		void truncate_cache_files();
		void create_directories();

	private:

		std::string m_file_name;
		std::string m_db_name;
		const size_t m_id;

		const size_t m_max_results;

		const size_t m_buffer_len = config::ft_shard_builder_buffer_len;
		char *m_buffer;
		std::mutex m_lock;

		// Caches
		std::vector<uint64_t> m_key_cache;
		std::vector<data_record> m_record_cache;

		std::map<uint64_t, vector<data_record>> m_cache;

		void read_append_cache();
		void read_data_to_cache();
		void sort_cache();
		void sort_record_list(uint64_t key, std::vector<data_record> &records);
		void reset_cache_variables();
		void save_file();
		void write_key(std::ofstream &key_writer, uint64_t key, size_t page_pos);
		size_t write_page(std::ofstream &writer, const std::vector<uint64_t> &keys);
		void reset_key_map(std::ofstream &key_writer);

		std::string mountpoint() const;
		std::string cache_filename() const;
		std::string key_cache_filename() const;
		std::string target_filename() const;
		std::string meta_filename() const;

	};

	template<typename data_record>
	basic_index_builder<data_record>::basic_index_builder(const std::string &file_name)
	: index_base<data_record>(), m_file_name(file_name), m_id(0),
		m_max_results(config::ft_max_results_per_section)
	{
		merger::register_merger((size_t)this, [this]() {merge();});
		merger::register_appender((size_t)this, [this]() {append();}, [this]() { return cache_size(); });
	}

	template<typename data_record>
	basic_index_builder<data_record>::basic_index_builder(const std::string &db_name, size_t id)
	: index_base<data_record>(), m_db_name(db_name), m_id(id), m_max_results(config::ft_max_results_per_section) {
		merger::register_merger((size_t)this, [this]() {merge();});
		merger::register_appender((size_t)this, [this]() {append();}, [this]() { return cache_size(); });
	}

	template<typename data_record>
	basic_index_builder<data_record>::basic_index_builder(const std::string &db_name, size_t id, size_t hash_table_size)
	: index_base<data_record>(hash_table_size), m_db_name(db_name), m_id(id), m_max_results(config::ft_max_results_per_section) {
		merger::register_merger((size_t)this, [this]() {append();});
		merger::register_appender((size_t)this, [this]() {append();}, [this]() { return cache_size(); });
	}

	template<typename data_record>
	basic_index_builder<data_record>::basic_index_builder(const std::string &db_name, size_t id, size_t hash_table_size, size_t max_results)
	: index_base<data_record>(hash_table_size), m_db_name(db_name), m_id(id), m_max_results(max_results) {
		merger::register_merger((size_t)this, [this]() {append();});
		merger::register_appender((size_t)this, [this]() {append();}, [this]() { return cache_size(); });
	}

	template<typename data_record>
	basic_index_builder<data_record>::~basic_index_builder() {
		merger::deregister_merger((size_t)this);
	}

	template<typename data_record>
	void basic_index_builder<data_record>::add(uint64_t key, const data_record &record) {

		indexer::merger::lock();

		m_lock.lock();

		// Amortized constant
		m_key_cache.push_back(key);
		m_record_cache.push_back(record);

		assert(m_record_cache.size() == m_key_cache.size());

		m_lock.unlock();

	}

	/*
	 * Returns the allocated size of the cache (m_key_cache and m_record_cache).
	 * */
	template<typename data_record>
	size_t basic_index_builder<data_record>::cache_size() const {
		return m_key_cache.capacity() * sizeof(uint64_t) + m_record_cache.capacity() * sizeof(data_record);
	}

	template<typename data_record>
	void basic_index_builder<data_record>::append() {

		assert(m_record_cache.size() == m_key_cache.size());

		std::ofstream record_writer(cache_filename(), std::ios::binary | std::ios::app);
		if (!record_writer.is_open()) {
			throw LOG_ERROR_EXCEPTION("Could not open full text shard (" + cache_filename() + "). Error: " +
				std::string(strerror(errno)));
		}

		std::ofstream key_writer(key_cache_filename(), std::ios::binary | std::ios::app);
		if (!key_writer.is_open()) {
			throw LOG_ERROR_EXCEPTION("Could not open full text shard (" + key_cache_filename() + "). Error: " +
				std::string(strerror(errno)));
		}

		record_writer.write((const char *)m_record_cache.data(), m_record_cache.size() * sizeof(data_record));
		key_writer.write((const char *)m_key_cache.data(), m_key_cache.size() * sizeof(uint64_t));

		m_record_cache.clear();
		m_key_cache.clear();
		m_record_cache.shrink_to_fit();
		m_key_cache.shrink_to_fit();
	}

	template<typename data_record>
	void basic_index_builder<data_record>::merge() {

		{
			read_append_cache();
			sort_cache();
			save_file();
			truncate_cache_files();
		}

	}

	/*
		Transforms all the bitmaps in the index. Basically generating new bitmaps with the transform applied.
	*/
	template<typename data_record>
	void basic_index_builder<data_record>::transform(const std::function<data_record(const data_record &, size_t)> &transform) {

		read_data_to_cache();

		// Apply transforms.
		for (auto &iter : m_cache) {
			for (size_t i = 0; i < iter.second.size(); i++) {
				iter.second[i] = transform(iter.second[i], iter.second.size());
			}
		}

		save_file();
		truncate_cache_files();
	}

	template<typename data_record>
	void basic_index_builder<data_record>::sort_by(const std::function<bool(const data_record &a, const data_record &b)> comp) {
		read_data_to_cache();

		for (auto &iter : m_cache) {
			sort(iter.second.begin(), iter.second.end(), comp);
		}

		save_file();
		truncate_cache_files();
	}

	/*
		Deletes ALL data from this shard.
	*/
	template<typename data_record>
	void basic_index_builder<data_record>::truncate() {
		create_directories();
		truncate_cache_files();

		std::ofstream target_writer(target_filename(), std::ios::trunc);
		target_writer.close();
	}

	/*
		Deletes all data from caches.
	*/
	template<typename data_record>
	void basic_index_builder<data_record>::truncate_cache_files() {

		reset_cache_variables();

		file::delete_file(cache_filename());
		file::delete_file(key_cache_filename());
	}

	template<typename data_record>
	void basic_index_builder<data_record>::create_directories() {
		for (size_t i = 0; i < 8; i++) {
			boost::filesystem::create_directories(config::data_path() + "/" + std::to_string(i) + "/full_text/" +
				m_db_name);
		}
	}

	template<typename data_record>
	void basic_index_builder<data_record>::read_append_cache() {

		// Read the current file.
		read_data_to_cache();

		//profiler::instance prof("index_builder::read_append_cache");

		// Read the cache into memory.
		std::ifstream reader(cache_filename(), std::ios::binary);
		if (!reader.is_open()) {
			throw LOG_ERROR_EXCEPTION("Could not open full text shard (" + cache_filename() + "). Error: " + std::string(strerror(errno)));
		}

		std::ifstream key_reader(key_cache_filename(), std::ios::binary);
		if (!key_reader.is_open()) {
			throw LOG_ERROR_EXCEPTION("Could not open full text shard (" + key_cache_filename() + "). Error: " + std::string(strerror(errno)));
		}

		const size_t buffer_len = 10000;

		std::unique_ptr<data_record[]> buffer_allocator;
		try {
			buffer_allocator = std::make_unique<data_record[]>(buffer_len);
		} catch (std::bad_alloc &exception) {
			std::cout << "bad_alloc detected: " << exception.what() << " file: " << __FILE__ << " line: " << __LINE__ << std::endl;
			std::cout << "tried to allocate: " << buffer_len * sizeof(data_record) << " bytes" << std::endl;
			return;
		}

		std::unique_ptr<uint64_t[]> key_buffer_allocator;
		try {
			key_buffer_allocator = std::make_unique<uint64_t[]>(buffer_len);
		} catch (std::bad_alloc &exception) {
			std::cout << "bad_alloc detected: " << exception.what() << " file: " << __FILE__ << " line: " << __LINE__ << std::endl;
			std::cout << "tried to allocate: " << buffer_len * sizeof(uint64_t) << " bytes" << std::endl;
			return;
		}

		data_record *buffer = buffer_allocator.get();
		uint64_t *key_buffer = key_buffer_allocator.get();

		reader.seekg(0, std::ios::beg);

		unordered_map<uint64_t, uint32_t> internal_id_map; 
		unordered_map<uint64_t, vector<uint32_t>> bitmap_data;

		while (!reader.eof()) {

			reader.read((char *)buffer, buffer_len * sizeof(data_record));
			key_reader.read((char *)key_buffer, buffer_len * sizeof(uint64_t));

			const size_t read_bytes = reader.gcount();
			const size_t num_records = read_bytes / sizeof(data_record);

			for (size_t i = 0; i < num_records; i++) {
				m_cache[key_buffer[i]].push_back(buffer[i]);
			}
		}
	}

	/*
	 * Reads the file into RAM.
	 * */
	template<typename data_record>
	void basic_index_builder<data_record>::read_data_to_cache() {

		reset_cache_variables();

		std::ifstream reader(target_filename(), std::ios::binary);
		if (!reader.is_open()) return;

		reader.seekg(0, std::ios::end);
		const size_t file_size = reader.tellg();
		if (file_size <= this->hash_table_byte_size()) return;
		reader.seekg(this->hash_table_byte_size(), std::ios::beg);

		while (this->read_page_into(reader, m_cache)) {
		}
	}

	template<typename data_record>
	void basic_index_builder<data_record>::sort_cache() {
		for (auto &iter : m_cache) {
			sort_record_list(iter.first, iter.second);
		}
	}

	template<typename data_record>
	void basic_index_builder<data_record>::sort_record_list(uint64_t key, std::vector<data_record> &records) {

		// Sort records.
		std::sort(records.begin(), records.end());

		// Sum equal elements.
		for (size_t i = 0, j = 1; i < records.size() && j < records.size(); j++) {
			if (records[i] != records[j]) {
				i = j;
			} else {
				records[i] += records[j];
			}
		}

		// Delete consecutive equal elements. Only keeping the first unique.
		auto last = std::unique(records.begin(), records.end());
		records.erase(last, records.end());


		if (records.size() > m_max_results) {
			// Sort before truncation
			std::sort(records.begin(), records.end(), typename data_record::truncate_order());
			records.resize(config::ft_max_results_per_section);

			// Future fix here is to add hyper log log counting for words with too many urls.
		}

		std::sort(records.begin(), records.end());
	}

	template<typename data_record>
	void basic_index_builder<data_record>::reset_cache_variables() {
		m_cache = std::map<uint64_t, vector<data_record>>{};
	}

	template<typename data_record>
	void basic_index_builder<data_record>::save_file() {

		//profiler::instance prof("index_builder::save_file");

		std::ofstream writer(target_filename(), std::ios::binary | std::ios::trunc);
		if (!writer.is_open()) {
			throw LOG_ERROR_EXCEPTION("Could not open full text shard. Error: " + std::string(strerror(errno)));
		}

		reset_key_map(writer);

		std::map<uint64_t, std::vector<uint64_t>> pages;
		for (auto &iter : m_cache) {
			if (this->m_hash_table_size) {
				pages[iter.first % this->m_hash_table_size].push_back(iter.first);
			} else {
				pages[0].push_back(iter.first);
			}
		}

		for (const auto &iter : pages) {
			size_t page_pos = write_page(writer, iter.second);
			write_key(writer, iter.first, page_pos);
			writer.flush();
		}
	}

	template<typename data_record>
	void basic_index_builder<data_record>::write_key(std::ofstream &key_writer, uint64_t key, size_t page_pos) {
		if (this->m_hash_table_size > 0) {
			assert(key < this->m_hash_table_size);
			key_writer.seekp(key * sizeof(uint64_t));
			key_writer.write((char *)&page_pos, sizeof(size_t));
		}
	}

	/*
	 * Writes the page with keys, appending it to the file stream writer.
	 * */
	template<typename data_record>
	size_t basic_index_builder<data_record>::write_page(std::ofstream &writer, const std::vector<uint64_t> &keys) {

		writer.seekp(0, ios::end);

		const size_t page_pos = writer.tellp();

		size_t num_keys = keys.size();

		writer.write((char *)&num_keys, 8);
		writer.write((char *)keys.data(), keys.size() * 8);

		std::vector<size_t> v_pos;
		std::vector<size_t> v_len;

		size_t pos = 0;
		for (uint64_t key : keys) {

			// Store position and length
			const size_t len = m_cache[key].size() * sizeof(data_record);
			
			v_pos.push_back(pos);
			v_len.push_back(len);

			pos += len;
		}
		
		writer.write((char *)v_pos.data(), keys.size() * 8);
		writer.write((char *)v_len.data(), keys.size() * 8);

		// Write data.
		size_t i = 0;
		for (uint64_t key : keys) {
			const size_t len = v_len[i];
			writer.write((char *)m_cache[key].data(), len);
			i++;
		}

		return page_pos;
	}

	template<typename data_record>
	void basic_index_builder<data_record>::reset_key_map(std::ofstream &key_writer) {
		key_writer.seekp(0);
		uint64_t data = SIZE_MAX;
		for (size_t i = 0; i < this->m_hash_table_size; i++) {
			key_writer.write((char *)&data, sizeof(uint64_t));
		}
	}

	template<typename data_record>
	std::string basic_index_builder<data_record>::mountpoint() const {
		return std::to_string(m_id % 8);
	}

	template<typename data_record>
	std::string basic_index_builder<data_record>::cache_filename() const {
		if (m_file_name != "") return m_file_name + ".cache";
		return config::data_path() + "/" + mountpoint() + "/full_text/" + m_db_name + "/" + std::to_string(m_id) +
			".cache";
	}

	template<typename data_record>
	std::string basic_index_builder<data_record>::key_cache_filename() const {
		if (m_file_name != "") return m_file_name + ".cache.keys";
		return config::data_path() + "/" + mountpoint() + "/full_text/" + m_db_name + "/" + std::to_string(m_id) +
			".cache.keys";
	}

	template<typename data_record>
	std::string basic_index_builder<data_record>::target_filename() const {
		if (m_file_name != "") return m_file_name + ".data";
		return config::data_path() + "/" + mountpoint() + "/full_text/" + m_db_name + "/" + std::to_string(m_id) +
			".data";
	}

}
