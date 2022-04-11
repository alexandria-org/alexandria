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
#include <cstring>
#include <cassert>
#include <boost/filesystem.hpp>
#include "merger.h"
#include "score_builder.h"
#include "algorithm/hyper_log_log.h"
#include "config.h"
#include "logger/logger.h"
#include "memory/debugger.h"
#include "roaring/roaring.hh"

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
	class index_builder {
	private:
		// Non copyable
		index_builder(const index_builder &);
		index_builder& operator=(const index_builder &);
	public:

		index_builder(const std::string &file_name);
		index_builder(const std::string &db_name, size_t id);
		index_builder(const std::string &db_name, size_t id, size_t hash_table_size);
		index_builder(const std::string &db_name, size_t id, size_t hash_table_size, size_t max_results);
		~index_builder();

		void add(uint64_t key, const data_record &record);
		
		void append();
		void merge();

		void truncate();
		void truncate_cache_files();
		void create_directories();

		/*void calculate_scores(algorithm algo, const score_builder &score);

		void calculate_scores_for_token(algorithm algo, const score_builder &score, uint64_t token,
			std::vector<data_record> &records);
		float calculate_score_for_record(algorithm algo, const score_builder &score, uint64_t token,
			const data_record &record);*/

		void set_hash_table_size(size_t size) { m_hash_table_size = size; }

	private:

		std::string m_file_name;
		std::string m_db_name;
		const size_t m_id;

		size_t m_hash_table_size;
		const size_t m_max_results;

		const size_t m_buffer_len = config::ft_shard_builder_buffer_len;
		char *m_buffer;
		std::mutex m_lock;

		// Id map
		map<uint64_t, uint16_t> m_id_map_16_bit;

		// Caches
		std::vector<uint64_t> m_key_cache;
		std::vector<data_record> m_record_cache;

		std::map<uint64_t, vector<data_record>> m_cache;

		std::vector<data_record> m_records;
		std::unordered_map<uint64_t, uint32_t> m_record_id_to_internal_id;
		std::map<uint64_t, roaring::Roaring> m_bitmaps;

		void read_append_cache();
		void read_data_to_cache();
		bool read_page(std::ifstream &reader, char *buffer, size_t buffer_len);
		void reset_cache_variables();
		void save_file();
		void write_key(std::ofstream &key_writer, uint64_t key, size_t page_pos);
		size_t write_page(std::ofstream &writer, const std::vector<uint64_t> &keys);
		void reset_key_map(std::ofstream &key_writer);
		void write_records(std::ofstream &writer);
		size_t hash_table_byte_size() const { return m_hash_table_size * sizeof(size_t); }
		void sort_cache();
		void sort_record_list(uint64_t key, std::vector<data_record> &records);

		std::string mountpoint() const;
		std::string cache_filename() const;
		std::string key_cache_filename() const;
		std::string target_filename() const;
		std::string meta_filename() const;

	};

	template<typename data_record>
	index_builder<data_record>::index_builder(const std::string &file_name)
	: m_file_name(file_name), m_id(0), m_hash_table_size(config::shard_hash_table_size), m_max_results(config::ft_max_results_per_section)
	{
		merger::register_merger((size_t)this, [this]() {merge();});
		merger::register_appender((size_t)this, [this]() {append();});
	}

	template<typename data_record>
	index_builder<data_record>::index_builder(const std::string &db_name, size_t id)
	: m_db_name(db_name), m_id(id), m_hash_table_size(config::shard_hash_table_size), m_max_results(config::ft_max_results_per_section) {
		merger::register_merger((size_t)this, [this]() {merge();});
		merger::register_appender((size_t)this, [this]() {append();});
	}

	template<typename data_record>
	index_builder<data_record>::index_builder(const std::string &db_name, size_t id, size_t hash_table_size)
	: m_db_name(db_name), m_id(id), m_hash_table_size(hash_table_size), m_max_results(config::ft_max_results_per_section) {
		merger::register_merger((size_t)this, [this]() {append();});
		merger::register_appender((size_t)this, [this]() {append();});
	}

	template<typename data_record>
	index_builder<data_record>::index_builder(const std::string &db_name, size_t id, size_t hash_table_size, size_t max_results)
	: m_db_name(db_name), m_id(id), m_hash_table_size(hash_table_size), m_max_results(max_results) {
		merger::register_merger((size_t)this, [this]() {append();});
		merger::register_appender((size_t)this, [this]() {append();});
	}

	template<typename data_record>
	index_builder<data_record>::~index_builder() {
		merger::deregister_merger((size_t)this);
	}

	template<typename data_record>
	void index_builder<data_record>::add(uint64_t key, const data_record &record) {

		indexer::merger::lock();

		m_lock.lock();

		// Amortized constant
		m_key_cache.push_back(key);
		m_record_cache.push_back(record);

		assert(m_record_cache.size() == m_key_cache.size());

		m_lock.unlock();

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

		//const size_t mem_before = memory::allocated_memory();

		{
			read_append_cache();
			save_file();
			truncate_cache_files();
		}

		//const size_t mem_usage = memory::get_usage();
		//const size_t mem_peak = memory::get_usage_peak();
		//const size_t mem_after = memory::allocated_memory();

		/*std::cout << "did merge: " << target_filename() << " mem bef: " << mem_before << " mem_after: " << mem_after << " mem_usage: " << mem_usage
			<< " mem_peak: " << mem_peak << std::endl;*/

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

		std::ofstream writer(cache_filename(), std::ios::trunc);
		writer.close();

		std::ofstream key_writer(key_cache_filename(), std::ios::trunc);
		key_writer.close();
	}

	template<typename data_record>
	void index_builder<data_record>::create_directories() {
		for (size_t i = 0; i < 8; i++) {
			boost::filesystem::create_directories("/mnt/" + std::to_string(i) + "/full_text/" + m_db_name);
		}
	}

	/*template<typename data_record>
	void index_builder<data_record>::calculate_scores(algorithm algo, const score_builder &score) {

		m_cache = std::map<uint64_t, std::vector<data_record>>{};

		// Read meta.
		std::unique_ptr<::algorithm::hyper_log_log> hll = std::make_unique<::algorithm::hyper_log_log>();
		read_meta(hll);

		// Read the current file.
		read_data_to_cache();

		for (auto &iter : m_cache) {
			calculate_scores_for_token(algo, score, iter.first, iter.second);
		}

		sort_cache();
		save_file();
		truncate_cache_files();
	}

	template<typename data_record>
	void index_builder<data_record>::calculate_scores_for_token(algorithm algo, const score_builder &score,
			uint64_t token, std::vector<data_record> &records) {
		for (data_record &record : records) {
			record.m_score = calculate_score_for_record(algo, score, token, record);
		}
	}

	template<typename data_record>
	float index_builder<data_record>::calculate_score_for_record(algorithm algo, const score_builder &score,
			uint64_t token, const data_record &record) {

		float val1 = (score.document_count() - total_results_for_key(token) + 0.5f);
		float val2 = (total_results_for_key(token) + 0.5f);
		float idf = log((val1 / val2) + 1.0f);

		if (algo == algorithm::bm25) {
			// reference: https://en.wikipedia.org/wiki/Okapi_BM25
			const float k1 = 1.2f;
			const float b = 0.75f;
			float tf = 0.0f;
			const size_t doc_size = score.document_size(record.m_value);
			if (doc_size) tf = (float)record.count() / doc_size;
			return idf * tf * (k1 + 1) / (tf + k1 * (1 - b + b * ((float)doc_size / score.avg_document_size())));
		}
		if (algo == algorithm::tf_idf) {
			// reference: https://en.wikipedia.org/wiki/Tf-idf
			float tf = 0.0f;
			const size_t doc_size = score.document_size(record.m_value);
			if (doc_size) tf = (float)record.count() / doc_size;
			return tf * idf;
		}

		return record.m_score;
	}*/

	template<typename data_record>
	void index_builder<data_record>::read_append_cache() {

		m_cache = std::map<uint64_t, std::vector<data_record>>{};

		// Read the current file.
		read_data_to_cache();

		// Read the cache into memory.
		std::ifstream reader(cache_filename(), std::ios::binary);
		if (!reader.is_open()) {
			throw LOG_ERROR_EXCEPTION("Could not open full text shard (" + cache_filename() + "). Error: " + std::string(strerror(errno)));
		}

		std::ifstream key_reader(key_cache_filename(), std::ios::binary);
		if (!key_reader.is_open()) {
			throw LOG_ERROR_EXCEPTION("Could not open full text shard (" + key_cache_filename() + "). Error: " + std::string(strerror(errno)));
		}

		const size_t buffer_len = 100000;
		const size_t buffer_size = sizeof(data_record) * buffer_len;
		const size_t key_buffer_size = sizeof(uint64_t) * buffer_len;

		std::unique_ptr<char[]> buffer_allocator;
		try {
			buffer_allocator = std::make_unique<char[]>(buffer_size);
		} catch (std::bad_alloc &exception) {
			std::cout << "bad_alloc detected: " << exception.what() << " file: " << __FILE__ << " line: " << __LINE__ << std::endl;
			std::cout << "tried to allocate: " << buffer_size << " bytes" << std::endl;
			return;
		}

		std::unique_ptr<char[]> key_buffer_allocator;
		try {
			key_buffer_allocator = std::make_unique<char[]>(key_buffer_size);
		} catch (std::bad_alloc &exception) {
			std::cout << "bad_alloc detected: " << exception.what() << " file: " << __FILE__ << " line: " << __LINE__ << std::endl;
			std::cout << "tried to allocate: " << key_buffer_size << " bytes" << std::endl;
			return;
		}

		char *buffer = buffer_allocator.get();
		char *key_buffer = key_buffer_allocator.get();

		reader.seekg(0, std::ios::beg);

		while (!reader.eof()) {

			reader.read(buffer, buffer_size);
			key_reader.read(key_buffer, key_buffer_size);

			const size_t read_bytes = reader.gcount();
			const size_t num_records = read_bytes / sizeof(data_record);

			for (size_t i = 0; i < num_records; i++) {
				const data_record *record = (data_record *)&buffer[i * sizeof(data_record)];
				const uint64_t key = *((uint64_t *)&key_buffer[i * sizeof(uint64_t)]);
				
				if (m_record_id_to_internal_id.count(record->m_value) == 0) {
					m_record_id_to_internal_id[record->m_value] = (uint32_t)m_records.size();
					m_records.push_back(*record);
				}
				const uint32_t internal_id = m_record_id_to_internal_id[record->m_value];
				m_bitmaps[key].add(internal_id);
			}
		}
	}

	/*
	 * Reads the file into RAM.
	 * */
	template<typename data_record>
	void index_builder<data_record>::read_data_to_cache() {

		reset_cache_variables();

		std::ifstream reader(target_filename(), std::ios::binary);
		if (!reader.is_open()) return;

		reader.seekg(0, std::ios::end);
		const size_t file_size = reader.tellg();
		if (file_size <= hash_table_byte_size()) return;
		reader.seekg(hash_table_byte_size(), std::ios::beg);

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
				m_records.push_back(record_buffer[i]);
			}
		}

		// Build record_to_internal_id map
		uint32_t internal_id = 0;
		for (const data_record &rec : m_records) {
			m_record_id_to_internal_id[rec.m_value] = internal_id++;
		}

		const size_t buffer_len = std::max(1000, (int)internal_id);
		std::unique_ptr<char[]> buffer_allocator;
		try {
			buffer_allocator = std::make_unique<char[]>(buffer_len);
		} catch (std::bad_alloc &exception) {
			std::cout << "bad_alloc detected: " << exception.what() << " file: " << __FILE__ << " line: " << __LINE__ << std::endl;
			std::cout << "tried to allocate: " << m_buffer_len << " bytes" << std::endl;
			return;
		}
		char *buffer = buffer_allocator.get();
		while (read_page(reader, buffer, buffer_len)) {
		}
	}

	template<typename data_record>
	bool index_builder<data_record>::read_page(std::ifstream &reader, char *buffer, size_t buffer_len) {

		reader.read(buffer, sizeof(uint64_t));
		if (reader.eof()) return false;

		uint64_t num_keys = *((uint64_t *)(&buffer[0]));

		std::unique_ptr<char[]> vector_buffer_allocator;
		try {
			vector_buffer_allocator = std::make_unique<char[]>(num_keys * sizeof(uint64_t));
		} catch (std::bad_alloc &exception) {
			std::cout << "bad_alloc detected: " << exception.what() << " file: " << __FILE__ << " line: " << __LINE__ << std::endl;
			std::cout << "tried to allocate: " << num_keys << " keys" << std::endl;
			return false;
		}

		char *vector_buffer = vector_buffer_allocator.get();

		// Read the keys.
		reader.read(vector_buffer, num_keys * sizeof(uint64_t));
		std::vector<uint64_t> keys;
		for (size_t i = 0; i < num_keys; i++) {
			keys.push_back(*((uint64_t *)(&vector_buffer[i*8])));
		}

		// Read the positions.
		reader.read(vector_buffer, num_keys * 8);
		std::vector<size_t> positions;
		for (size_t i = 0; i < num_keys; i++) {
			positions.push_back(*((size_t *)(&vector_buffer[i*8])));
		}

		// Read the lengths.
		reader.read(vector_buffer, num_keys * 8);
		std::vector<size_t> lens;
		size_t max_len = 0;
		size_t data_size = 0;
		for (size_t i = 0; i < num_keys; i++) {
			size_t len = *((size_t *)(&vector_buffer[i*8]));
			if (len > max_len) max_len = len;
			lens.push_back(len);
			data_size += len;
		}

		if (data_size == 0) return true;

		if (max_len > buffer_len) {
			throw out_of_range("buffer_len is too small in read_page");
		}

		// Read the bitmap data.
		for (size_t i = 0; i < num_keys; i++) {
			const size_t len = lens[i];
			reader.read(buffer, len);
			const size_t read_len = reader.gcount();
			if (read_len != len) {
				LOG_INFO("Data stopped before end. Ignoring shard " + m_id);
				reset_cache_variables();
				break;
			}

			m_bitmaps[keys[i]] = roaring::Roaring::readSafe(buffer, len);
		}

		return true;
	}

	template<typename data_record>
	void index_builder<data_record>::reset_cache_variables() {
		m_records = std::vector<data_record>{};
		m_record_id_to_internal_id = std::unordered_map<uint64_t, uint32_t>{};
		m_bitmaps = std::map<uint64_t, roaring::Roaring>{};
	}

	template<typename data_record>
	void index_builder<data_record>::save_file() {

		std::ofstream writer(target_filename(), std::ios::binary | std::ios::trunc);
		if (!writer.is_open()) {
			throw LOG_ERROR_EXCEPTION("Could not open full text shard. Error: " + std::string(strerror(errno)));
		}

		reset_key_map(writer);
		write_records(writer);

		std::map<uint64_t, std::vector<uint64_t>> pages;
		for (auto &iter : m_bitmaps) {
			if (m_hash_table_size) {
				pages[iter.first % m_hash_table_size].push_back(iter.first);
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
		if (m_hash_table_size > 0) {
			assert(key < m_hash_table_size);
			key_writer.seekp(key * sizeof(uint64_t));
			key_writer.write((char *)&page_pos, sizeof(size_t));
		}
	}

	/*
	 * Writes the page with keys, appending it to the file stream writer. Takes data from m_cache.
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
		for (size_t i = 0; i < m_hash_table_size; i++) {
			key_writer.write((char *)&data, sizeof(uint64_t));
		}
	}

	template<typename data_record>
	void index_builder<data_record>::write_records(std::ofstream &writer) {
		const size_t num_records = m_records.size();
		writer.write((char *)&num_records, sizeof(uint64_t));
		writer.write((char *)m_records.data(), num_records * sizeof(data_record));
	}

	template<typename data_record>
	void index_builder<data_record>::sort_cache() {
		for (auto &iter : m_cache) {
			sort_record_list(iter.first, iter.second);
		}
	}

	template<typename data_record>
	void index_builder<data_record>::sort_record_list(uint64_t key, std::vector<data_record> &records) {
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

		// Delete consecutive elements. Only keeping the first.
		auto last = std::unique(records.begin(), records.end());
		records.erase(last, records.end());

		if (records.size() > m_max_results) {

			/*
				These results should be truncated.
				Then we need to create a hyper log log object that can keep track of total number of results.
				This should be OK since the table already takes up a lot of memory, so adding 15K hyper log log
				registers should not blow up.
				For example m_max_results should be around 2M long
			
			auto total_counter = get_total_counter_for_key(key);

			for (const data_record &record : records) {
				total_counter->insert(record.m_value);
			}

			// Sort all the records by score, so that we truncate away everything with lowest score.
			std::sort(records.begin(), records.end(), [](const data_record &a, const data_record &b) {
				return a.m_score > b.m_score;
			});

			// Truncate everything with low score.
			if (records.size() > m_max_results) {
				records.resize(m_max_results);
			}*/
		}

		// Order by storage_order.
		std::sort(records.begin(), records.end(), typename data_record::storage_order());

		records.shrink_to_fit(); // Save memory.
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

}
