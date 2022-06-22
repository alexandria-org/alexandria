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
#include <numeric>
#include <boost/filesystem.hpp>
#include "merger.h"
#include "score_builder.h"
#include "index_utils.h"
#include "index_base.h"
#include "index.h"
#include "algorithm/hyper_log_log.h"
#include "config.h"
#include "profiler/profiler.h"
#include "logger/logger.h"
#include "file/file.h"
#include "memory/debugger.h"
#include "roaring/roaring.hh"
#include "URL.h"

namespace indexer {

	/*
		<hash-table-data> uint8_t[hash_table_size]
		<num-records> uint64_t
		<records> data_record[num-records] sequence of records, the position of the record is the internal_id
		<page-data> page[num_pages]

		page format:
		<num_keys> uint64_t
		<key-data> uint64_t[num_keys] sorted by key for binary search
		<pos-data> uint64_t[num_keys] position of record data start
		<len-data> uint64_t[num_keys] length of record data
		<record-data> <bitmap>[num_keys] bitmap is a roaring bitmap (CRoaring)
	*/

	enum class algorithm { bm25 = 101, tf_idf = 102};

	template<typename data_record>
	class index_builder : public index_base<data_record> {
	private:
		// Non copyable
		index_builder(const index_builder &);
		index_builder& operator=(const index_builder &);
	public:

		index_builder(const std::string &file_name);
		index_builder(const std::string &db_name, size_t id);
		index_builder(const std::string &db_name, size_t id, size_t hash_table_size);
		index_builder(const std::string &db_name, size_t id, size_t hash_table_size, size_t max_results);
		index_builder(const std::string &db_name, size_t id, std::function<uint32_t(const data_record &)> &rec_to_id);
		~index_builder();

		void add(uint64_t key, const data_record &record);
		size_t cache_size() const;
		void transform(const std::function<uint32_t(uint32_t)> &transform);
		
		void append();
		void merge();
		void merge(std::unordered_map<uint64_t, uint32_t> &internal_id_map);
		void merge_with(const index<data_record> &other);
		void optimize();

		void truncate();
		void truncate_cache_files();
		void create_directories();

		/*void calculate_scores(algorithm algo, const score_builder &score);

		void calculate_scores_for_token(algorithm algo, const score_builder &score, uint64_t token,
			std::vector<data_record> &records);
		float calculate_score_for_record(algorithm algo, const score_builder &score, uint64_t token,
			const data_record &record);*/

		size_t get_max_id();

		static void create_directories(const std::string &db_name);

	private:

		std::string m_file_name;
		std::string m_db_name;
		const size_t m_id;

		const size_t m_max_results;

		std::mutex m_lock;

		// Caches
		std::vector<uint64_t> m_key_cache;
		std::vector<data_record> m_record_cache;
		

		std::vector<data_record> m_records;
		std::map<uint64_t, uint32_t> m_record_id_map;
		std::map<uint64_t, roaring::Roaring> m_bitmaps;

		std::function<uint32_t(const data_record &)> m_record_id_to_internal_id = [this](const data_record &record) {
			if (m_record_id_map.count(record.m_value) == 0) {
				m_record_id_map[record.m_value] = m_records.size();
				m_records.push_back(record);
			}
			return m_record_id_map[record.m_value];
		};

		void read_append_cache();
		void read_append_cache(std::unordered_map<uint64_t, uint32_t> &internal_id_map);
		void read_data_to_cache();
		bool read_page(std::ifstream &reader);
		void reset_cache_variables();
		void save_file();
		void write_key(std::ofstream &key_writer, uint64_t key, size_t page_pos);
		size_t write_page(std::ofstream &writer, const std::vector<uint64_t> &keys);
		void reset_key_map(std::ofstream &key_writer);
		std::vector<data_record> read_records() const;
		void write_records(std::ofstream &writer);
		uint32_t default_record_to_internal_id(const data_record &record);

		std::string mountpoint() const;
		std::string cache_filename() const;
		std::string key_cache_filename() const;
		std::string target_filename() const;
		std::string meta_filename() const;

		bool needs_optimization() const;
		void sort_records();

	};

	template<typename data_record>
	index_builder<data_record>::index_builder(const std::string &file_name)
	: index_base<data_record>(), m_file_name(file_name), m_id(0),
		m_max_results(config::ft_max_results_per_section)
	{
		merger::register_merger((size_t)this, [this]() {merge();});
		merger::register_appender((size_t)this, [this]() {append();}, [this]() { return cache_size(); });
	}

	template<typename data_record>
	index_builder<data_record>::index_builder(const std::string &db_name, size_t id)
	: index_base<data_record>(), m_db_name(db_name), m_id(id), m_max_results(config::ft_max_results_per_section) {
		merger::register_merger((size_t)this, [this]() {merge();});
		merger::register_appender((size_t)this, [this]() {append();}, [this]() { return cache_size(); });
	}

	template<typename data_record>
	index_builder<data_record>::index_builder(const std::string &db_name, size_t id, size_t hash_table_size)
	: index_base<data_record>(hash_table_size), m_db_name(db_name), m_id(id), m_max_results(config::ft_max_results_per_section) {
		merger::register_merger((size_t)this, [this]() {merge();});
		merger::register_appender((size_t)this, [this]() {append();}, [this]() { return cache_size(); });
	}

	template<typename data_record>
	index_builder<data_record>::index_builder(const std::string &db_name, size_t id, size_t hash_table_size, size_t max_results)
	: index_base<data_record>(hash_table_size), m_db_name(db_name), m_id(id), m_max_results(max_results) {
		merger::register_merger((size_t)this, [this]() {merge();});
		merger::register_appender((size_t)this, [this]() {append();}, [this]() { return cache_size(); });
	}

	template<typename data_record>
	index_builder<data_record>::index_builder(const std::string &db_name, size_t id,
		std::function<uint32_t(const data_record &)> &rec_to_id)
	: index_base<data_record>(), m_db_name(db_name), m_id(id), m_max_results(config::ft_max_results_per_section) {
		m_record_id_to_internal_id = rec_to_id;
		merger::register_merger((size_t)this, [this]() {merge();});
		merger::register_appender((size_t)this, [this]() {append();}, [this]() { return cache_size(); });
	}

	template<typename data_record>
	index_builder<data_record>::~index_builder() {
		merger::deregister_merger((size_t)this);
	}

	template<typename data_record>
	void index_builder<data_record>::add(uint64_t key, const data_record &record) {
		indexer::merger::lock();

		std::lock_guard guard(m_lock);

		// Amortized constant
		m_key_cache.push_back(key);
		m_record_cache.push_back(record);

	}

	/*
	 * Returns the allocated size of the cache (m_key_cache and m_record_cache).
	 * */
	template<typename data_record>
	size_t index_builder<data_record>::cache_size() const {
		return m_key_cache.capacity() * sizeof(uint64_t) + m_record_cache.capacity() * sizeof(data_record);
	}

	/*
		Transforms all the bitmaps in the index. Basically generating new bitmaps with the transform applied.
	*/
	template<typename data_record>
	void index_builder<data_record>::transform(const std::function<uint32_t(uint32_t)> &transform) {
		read_data_to_cache();

		// Apply transforms.
		for (auto &iter : m_bitmaps) {

			::roaring::Roaring rr;
			for (uint32_t v : iter.second) {
				const uint32_t v_trans = transform(v);
				rr.add(v_trans);
			}
			m_bitmaps[iter.first] = rr;
		}

		save_file();
		truncate_cache_files();
	}

	template<typename data_record>
	void index_builder<data_record>::append() {

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
	void index_builder<data_record>::merge() {
		std::unordered_map<uint64_t, uint32_t> internal_id_map;
		merge(internal_id_map);
	}

	template<typename data_record>
	void index_builder<data_record>::merge(std::unordered_map<uint64_t, uint32_t> &internal_id_map) {

		{
			read_append_cache(internal_id_map);
			save_file();
			truncate_cache_files();
		}

	}

	template<typename data_record>
	void index_builder<data_record>::merge_with(const index<data_record> &other) {
		/*
		 * The only algorithm I can come up with is to append the records from 'other' that are not present in 'this'.
		 * And also create a map from ids in 'other' to ids in the new record array.
		 * Then transform the bitmaps in other before merging them.
		 * */

		const auto &other_records = other.records();

		typename data_record::storage_order ordered;

		if (!std::is_sorted(other_records.cbegin(), other_records.cend(), ordered))
			throw std::runtime_error("index_builder::merge_with needs optimized input");

		read_data_to_cache();

		if (!std::is_sorted(m_records.cbegin(), m_records.cend(), ordered))
			throw std::runtime_error("index_builder::merge_with needs to run on optimized index");

		std::map<uint32_t, uint32_t> id_map;
		std::vector<data_record> new_records;

		size_t i = 0, j = 0;
		while (i < m_records.size() && j < other_records.size()) {
			if (ordered(m_records[i], other_records[j])) {
				i++;
			} else if (m_records[i].storage_equal(other_records[j])) {
				id_map[j] = i;
				i++;
				j++;
			} else {
				id_map[j] = m_records.size() + new_records.size();
				new_records.push_back(other_records[j]);
				j++;
			}
		}
		while (j < other_records.size()) {
			id_map[j] = m_records.size() + new_records.size();
			new_records.push_back(other_records[j]);
			j++;
		}

		m_records.insert(m_records.end(), new_records.cbegin(), new_records.cend());

		other.for_each([this, &id_map](uint64_t key, roaring::Roaring &bitmap) {
			roaring::Roaring new_bitmap;
			for (auto idx : bitmap) {
				new_bitmap.add(id_map[idx]);
			}
			m_bitmaps[key] |= new_bitmap;
		});

		save_file();
		truncate_cache_files();

		optimize();
	}

	template<typename data_record>
	void index_builder<data_record>::optimize() {
		if (needs_optimization()) {
			sort_records();
		}
	}

	/*
		Deletes ALL data from this shard.
	*/
	template<typename data_record>
	void index_builder<data_record>::truncate() {
		create_directories();
		truncate_cache_files();

		std::ofstream target_writer(target_filename(), std::ios::trunc);
		target_writer.close();
	}

	/*
		Deletes all data from caches.
	*/
	template<typename data_record>
	void index_builder<data_record>::truncate_cache_files() {

		reset_cache_variables();

		file::delete_file(cache_filename());
		file::delete_file(key_cache_filename());

	}

	template<typename data_record>
	void index_builder<data_record>::create_directories() {
		create_db_directories(m_db_name);
	}

	template<typename data_record>
	size_t index_builder<data_record>::get_max_id() {

		read_data_to_cache();

		uint32_t max_internal_id = 0;
		for (const auto &iter : m_bitmaps) {
			uint32_t internal_id = iter.second.maximum();
			if (internal_id > max_internal_id) {
				max_internal_id = internal_id;
			}
		}

		return (size_t)max_internal_id;
	}

	template<typename data_record>
	void index_builder<data_record>::create_directories(const std::string &db_name) {
		for (size_t i = 0; i < 8; i++) {
			file::create_directory("/mnt/" + std::to_string(i) + "/full_text/" + db_name);
		}
	}

	template<typename data_record>
	void index_builder<data_record>::read_append_cache() {
		std::unordered_map<uint64_t, uint32_t> internal_id_map;
		read_append_cache(internal_id_map);
	}

	template<typename data_record>
	void index_builder<data_record>::read_append_cache(std::unordered_map<uint64_t, uint32_t> &internal_id_map) {

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

		std::unordered_map<uint64_t, vector<uint32_t>> bitmap_data;

		while (!reader.eof()) {

			reader.read((char *)buffer, buffer_len * sizeof(data_record));
			key_reader.read((char *)key_buffer, buffer_len * sizeof(uint64_t));

			const size_t read_bytes = reader.gcount();
			const size_t num_records = read_bytes / sizeof(data_record);

			for (size_t i = 0; i < num_records; i++) {
				const auto map_iter = internal_id_map.find(buffer[i].m_value);
				if (map_iter == internal_id_map.end()) {
					const uint32_t internal_id = m_record_id_to_internal_id(buffer[i]);
					internal_id_map[buffer[i].m_value] = internal_id;
					bitmap_data[key_buffer[i]].push_back(internal_id);
				} else {
					bitmap_data[key_buffer[i]].push_back(map_iter->second);
				}
			}
		}

		// Insert the bitmap data.
		for (const auto &iter : bitmap_data) {
			m_bitmaps[iter.first].addMany(iter.second.size(), iter.second.data());
		}
	}

	/*
	 * Reads the file into RAM.
	 * */
	template<typename data_record>
	void index_builder<data_record>::read_data_to_cache() {

		//profiler::instance prof("index_builder::read_data_to_cache");

		reset_cache_variables();

		std::ifstream reader(target_filename(), std::ios::binary);
		if (!reader.is_open()) return;

		reader.seekg(0, std::ios::end);
		const size_t file_size = reader.tellg();
		if (file_size <= this->hash_table_byte_size()) return;
		reader.seekg(this->hash_table_byte_size(), std::ios::beg);

		size_t num_records;
		reader.read((char *)&num_records, sizeof(size_t));

		// Read records.
		const size_t record_buffer_len = 10000;
		std::unique_ptr<data_record[]> record_buffer_allocator = std::make_unique<data_record[]>(record_buffer_len);
		data_record *record_buffer = record_buffer_allocator.get();

		size_t records_read = 0;
		while (records_read < num_records) {
			size_t records_left = num_records - records_read;
			size_t records_to_read = min(records_left, record_buffer_len);
			reader.read((char *)record_buffer, sizeof(data_record) * records_to_read);

			for (size_t i = 0; i < records_to_read; i++) {
				m_record_id_map[record_buffer[i].m_value] = m_records.size();
				m_records.push_back(record_buffer[i]);
			}

			records_read += records_to_read;
		}

		while (this->read_bitmap_page_into(reader, m_bitmaps)) {
		}
	}

	template<typename data_record>
	void index_builder<data_record>::reset_cache_variables() {
		m_records = std::vector<data_record>{};
		m_record_id_map = std::map<uint64_t, uint32_t>{};
		m_bitmaps = std::map<uint64_t, roaring::Roaring>{};
	}

	template<typename data_record>
	void index_builder<data_record>::save_file() {

		//profiler::instance prof("index_builder::save_file");

		std::ofstream writer(target_filename(), std::ios::binary | std::ios::trunc);
		if (!writer.is_open()) {
			throw LOG_ERROR_EXCEPTION("Could not open full text shard. Error: " + std::string(strerror(errno)));
		}

		reset_key_map(writer);
		write_records(writer);

		std::map<uint64_t, std::vector<uint64_t>> pages;
		for (auto &iter : m_bitmaps) {
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
	void index_builder<data_record>::write_key(std::ofstream &key_writer, uint64_t key, size_t page_pos) {
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
	size_t index_builder<data_record>::write_page(std::ofstream &writer, const std::vector<uint64_t> &keys) {

		writer.seekp(0, ios::end);

		const size_t page_pos = writer.tellp();

		size_t num_keys = keys.size();

		writer.write((char *)&num_keys, 8);
		writer.write((char *)keys.data(), keys.size() * 8);

		std::vector<size_t> v_pos;
		std::vector<size_t> v_len;

		size_t max_len = 0;
		size_t pos = 0;
		for (uint64_t key : keys) {

			m_bitmaps[key].runOptimize();
			m_bitmaps[key].shrinkToFit();

			// Store position and length
			const size_t len = m_bitmaps[key].getSizeInBytes();

			if (len > max_len) max_len = len;
			
			v_pos.push_back(pos);
			v_len.push_back(len);

			pos += len;
		}
		
		writer.write((char *)v_pos.data(), keys.size() * 8);
		writer.write((char *)v_len.data(), keys.size() * 8);

		std::unique_ptr<char[]> buffer_allocator = make_unique<char[]>(max_len);
		char *buffer = buffer_allocator.get();

		// Write data.
		for (uint64_t key : keys) {
			const size_t len = m_bitmaps[key].getSizeInBytes();
			m_bitmaps[key].write(buffer);
			writer.write(buffer, len);
		}

		return page_pos;
	}

	template<typename data_record>
	void index_builder<data_record>::reset_key_map(std::ofstream &key_writer) {
		key_writer.seekp(0);
		uint64_t data = SIZE_MAX;
		for (size_t i = 0; i < this->m_hash_table_size; i++) {
			key_writer.write((char *)&data, sizeof(uint64_t));
		}
	}

	template<typename data_record>
	std::vector<data_record> index_builder<data_record>::read_records() const {
		ifstream reader(target_filename(), std::ios::in);
		reader.seekg(this->hash_table_byte_size(), std::ios::beg);

		const size_t num_records = m_records.size();
		reader.read((char *)&num_records, sizeof(uint64_t));

		std::vector<data_record> records(num_records);
		reader.read((char *)records.data(), num_records * sizeof(data_record));

		return records;
	}

	template<typename data_record>
	void index_builder<data_record>::write_records(std::ofstream &writer) {
		const size_t num_records = m_records.size();
		writer.write((char *)&num_records, sizeof(uint64_t));
		writer.write((char *)m_records.data(), num_records * sizeof(data_record));
	}

	template<typename data_record>
	uint32_t index_builder<data_record>::default_record_to_internal_id(const data_record &record) {
		if (m_record_id_map.count(record.m_value) == 0) {
			m_record_id_map[record.m_value] = m_records.size();
			m_records.push_back(record);
		}
		return m_record_id_map[record.m_value];
	}

	template<typename data_record>
	std::string index_builder<data_record>::mountpoint() const {
		return std::to_string(m_id % 8);
	}

	template<typename data_record>
	std::string index_builder<data_record>::cache_filename() const {
		if (m_file_name != "") return m_file_name + ".cache";
		return "/mnt/" + mountpoint() + "/full_text/" + m_db_name + "/" + std::to_string(m_id) + ".cache";
	}

	template<typename data_record>
	std::string index_builder<data_record>::key_cache_filename() const {
		if (m_file_name != "") return m_file_name + ".cache.keys";
		return "/mnt/" + mountpoint() + "/full_text/" + m_db_name + "/" + std::to_string(m_id) +".cache.keys";
	}

	template<typename data_record>
	std::string index_builder<data_record>::target_filename() const {
		if (m_file_name != "") return m_file_name + ".data";
		return "/mnt/" + mountpoint() + "/full_text/" + m_db_name + "/" + std::to_string(m_id) + ".data";
	}

	template<typename data_record>
	bool index_builder<data_record>::needs_optimization() const {

		auto records = read_records();

		// Just check if the records are sorted by storage order.
		if (records.size() <= 1) return false;
		
		return !std::is_sorted(records.cbegin(), records.cend(), typename data_record::storage_order());
	}

	template<typename data_record>
	void index_builder<data_record>::sort_records() {

		read_data_to_cache();

		std::vector<uint32_t> permutation(m_records.size());
		std::iota(permutation.begin(), permutation.end(), 0);

		typename data_record::storage_order ordered;

		std::sort(permutation.begin(), permutation.end(), [this, &ordered](const size_t &a, const size_t &b) {
			return ordered(m_records[a], m_records[b]);
		});
		// permutation now points from new position -> old position of record.

		std::vector<uint32_t> inverse(permutation.size());
		for (uint32_t i = 0; i < permutation.size(); i++) {
			inverse[permutation[i]] = i;
		}
		// inverse now points from old position -> new position of record.

		// Reorder the records.
		sort(m_records.begin(), m_records.end(), ordered);

		// Apply transforms.
		for (auto &iter : m_bitmaps) {

			::roaring::Roaring rr;
			for (uint32_t v : iter.second) {
				const uint32_t v_trans = inverse[v];
				rr.add(v_trans);
			}
			m_bitmaps[iter.first] = rr;
		}

		save_file();
		truncate_cache_files();
	}

}
