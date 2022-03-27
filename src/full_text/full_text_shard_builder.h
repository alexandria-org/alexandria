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

namespace full_text {
	template<typename data_record> class full_text_shard_builder;
}

#include "full_text_indexer.h"
#include "full_text_index.h"
#include "parser/URL.h"
#include "full_text_record.h"
#include "url_to_domain.h"
#include "logger/logger.h"

namespace full_text {

	template<typename data_record>
	class full_text_shard_builder {

		public:

			full_text_shard_builder(const std::string &db_name, size_t shard_id);
			full_text_shard_builder(const std::string &db_name, size_t shard_id, size_t bytes_per_shard);
			~full_text_shard_builder();

			void add(uint64_t key, const data_record &record);
			void sort_cache();
			bool full() const;
			bool over_full() const;
			void append();
			void merge();
			bool should_merge();
			void merge_with(full_text_shard_builder<data_record> &with);

			std::string mountpoint() const;
			std::string cache_filename() const;
			std::string key_cache_filename() const;
			std::string key_filename() const;
			std::string target_filename() const;

			void truncate();
			void truncate_cache_files();

			size_t disk_size() const;
			size_t cache_size() const;

		private:

			const std::string m_db_name;
			const size_t m_shard_id;
			const size_t m_max_cache_size;

			const size_t m_max_cache_file_size = 300 * 1000 * 1000; // 200mb.
			const size_t m_max_num_keys = 10000;
			const size_t m_buffer_len = Config::ft_shard_builder_buffer_len;
			char *m_buffer;

			std::vector<uint64_t> m_keys;
			std::vector<data_record> m_records;

			std::map<uint64_t, std::vector<data_record>> m_cache;
			std::map<uint64_t, size_t> m_total_results;

			void read_append_cache();
			void read_data_to_cache();
			bool read_page(std::ifstream &reader);
			void save_file();
			void write_key(std::ofstream &key_writer, uint64_t key, size_t page_pos);
			size_t write_page(std::ofstream &writer, const std::vector<uint64_t> &keys);
			void reset_key_file(std::ofstream &key_writer);
			void order_sections_by_value(std::vector<data_record> &results) const;

	};

	template<typename data_record>
	full_text_shard_builder<data_record>::full_text_shard_builder(const std::string &db_name, size_t shard_id)
	: m_db_name(db_name), m_shard_id(shard_id), m_max_cache_size(Config::ft_cached_bytes_per_shard() / sizeof(data_record)) {
	}

	template<typename data_record>
	full_text_shard_builder<data_record>::full_text_shard_builder(const std::string &db_name, size_t shard_id, size_t bytes_per_shard)
	: m_db_name(db_name), m_shard_id(shard_id), m_max_cache_size(bytes_per_shard / sizeof(data_record)) {
	}

	template<typename data_record>
	full_text_shard_builder<data_record>::~full_text_shard_builder() {
	}

	template<typename data_record>
	void full_text_shard_builder<data_record>::add(uint64_t key, const data_record &record) {

		// Amortized constant
		m_keys.push_back(key);
		m_records.push_back(record);

	}

	template<typename data_record>
	void full_text_shard_builder<data_record>::sort_cache() {
		const size_t max_results = Config::ft_max_results_per_section * Config::ft_max_sections;
		for (auto &iter : m_cache) {
			// Make elements unique.
			std::sort(iter.second.begin(), iter.second.end(), [](const data_record &a, const data_record &b) {
				return a.m_value < b.m_value;
			});
			auto last = std::unique(iter.second.begin(), iter.second.end(),
				[](const data_record &a, const data_record &b) {
				return a.m_value == b.m_value;
			});
			iter.second.erase(last, iter.second.end());

			m_total_results[iter.first] = iter.second.size();

			if (iter.second.size() > Config::ft_max_results_per_section) {
				std::sort(iter.second.begin(), iter.second.end(), [](const data_record &a, const data_record &b) {
					return a.m_score > b.m_score;
				});

				// Cap results at the maximum number of results.
				if (iter.second.size() > max_results) {
					iter.second.resize(max_results);
				}

				// Order each section by value.
				order_sections_by_value(iter.second);
			}
		}
	}

	template<typename data_record>
	void full_text_shard_builder<data_record>::order_sections_by_value(std::vector<data_record> &results) const {
		bool stop = false;
		for (size_t section = 0; section < Config::ft_max_sections; section++) {
			const size_t start = section * Config::ft_max_results_per_section;
			size_t end = start + Config::ft_max_results_per_section;
			if (end > results.size()) {
				end = results.size();
				stop = true;
			}
			std::sort(results.begin() + start, results.begin() + end, [](const data_record &a, const data_record &b) {
				return a.m_value < b.m_value;
			});
			if (stop) break;
		}
	}

	template<typename data_record>
	bool full_text_shard_builder<data_record>::full() const {
		return cache_size() > m_max_cache_size;
	}

	template<typename data_record>
	bool full_text_shard_builder<data_record>::over_full() const {
		return cache_size() > 2*m_max_cache_size;
	}

	template<typename data_record>
	void full_text_shard_builder<data_record>::append() {
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

		record_writer.write((const char *)m_records.data(), m_records.size() * sizeof(data_record));
		key_writer.write((const char *)m_keys.data(), m_keys.size() * sizeof(uint64_t));

		m_records.clear();
		m_keys.clear();
		m_records.shrink_to_fit();
		m_keys.shrink_to_fit();
	}

	template<typename data_record>
	void full_text_shard_builder<data_record>::merge() {

		read_append_cache();
		sort_cache();
		save_file();
		truncate_cache_files();

	}

	template<typename data_record>
	void full_text_shard_builder<data_record>::read_append_cache() {

		m_cache.clear();
		m_total_results.clear();

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
		char *buffer;
		char *key_buffer;
		try {
			buffer = new char[buffer_size];
		} catch (std::bad_alloc &exception) {
			std::cout << "bad_alloc detected: " << exception.what() << " file: " << __FILE__ << " line: " << __LINE__ << std::endl;
			std::cout << "tried to allocate: " << buffer_size << " bytes" << std::endl;
			return;
		}
		try {
			key_buffer = new char[key_buffer_size];
		} catch (std::bad_alloc &exception) {
			std::cout << "bad_alloc detected: " << exception.what() << " file: " << __FILE__ << " line: " << __LINE__ << std::endl;
			std::cout << "tried to allocate: " << key_buffer_size << " bytes" << std::endl;
			return;
		}

		reader.seekg(0, std::ios::beg);

		while (!reader.eof()) {

			reader.read(buffer, buffer_size);
			key_reader.read(key_buffer, key_buffer_size);

			const size_t read_bytes = reader.gcount();
			const size_t num_records = read_bytes / sizeof(data_record);

			for (size_t i = 0; i < num_records; i++) {
				const data_record *record = (data_record *)&buffer[i * sizeof(data_record)];
				const uint64_t key = *((uint64_t *)&key_buffer[i * sizeof(uint64_t)]);
				m_cache[key].push_back(*record);
			}
		}

		delete [] key_buffer;
		delete [] buffer;
	}

	template<typename data_record>
	bool full_text_shard_builder<data_record>::should_merge() {

		std::ofstream writer(cache_filename(), std::ios::binary | std::ios::app);
		size_t cache_file_size = writer.tellp();

		return cache_file_size > m_max_cache_file_size;
	}

	template<typename data_record>
	void full_text_shard_builder<data_record>::merge_with(full_text_shard_builder<data_record> &with) {

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
			const std::vector<data_record> *vec1 = &iter.second;
			const std::vector<data_record> *vec2 = &(with.m_cache[iter.first]);
			size_t i = 0;
			size_t j = 0;
			std::vector<data_record> merged;
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
	template<typename data_record>
	void full_text_shard_builder<data_record>::read_data_to_cache() {

		m_cache.clear();
		m_total_results.clear();

		std::ifstream reader(target_filename(), std::ios::binary);
		if (!reader.is_open()) return;

		reader.seekg(0, std::ios::end);
		const size_t file_size = reader.tellg();
		if (file_size == 0) return;
		reader.seekg(0, std::ios::beg);

		try {
			m_buffer = new char[m_buffer_len];
		} catch (std::bad_alloc &exception) {
			std::cout << "bad_alloc detected: " << exception.what() << " file: " << __FILE__ << " line: " << __LINE__ << std::endl;
			std::cout << "tried to allocate: " << m_buffer_len << " bytes" << std::endl;
			return;
		}
		while (read_page(reader)) {
		}
		delete m_buffer;
	}

	template<typename data_record>
	bool full_text_shard_builder<data_record>::read_page(std::ifstream &reader) {

		char buffer[64];

		reader.read(buffer, 8);

		if (reader.eof()) return false;

		uint64_t num_keys = *((uint64_t *)(&buffer[0]));

		char *vector_buffer;
		try {
			vector_buffer = new char[num_keys * 8];
		} catch (std::bad_alloc &exception) {
			std::cout << "bad_alloc detected: " << exception.what() << " file: " << __FILE__ << " line: " << __LINE__ << std::endl;
			std::cout << "tried to allocate: " << num_keys << " keys" << std::endl;
			return false;
		}

		// Read the keys.
		reader.read(vector_buffer, num_keys * 8);
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
		size_t num_records_for_key = lens[key_id] / sizeof(data_record);
		while (total_read_data < data_size) {
			const size_t to_read_now = std::min(m_buffer_len, data_size - total_read_data);
			reader.read(m_buffer, to_read_now);
			const size_t read_len = reader.gcount();

			if (read_len == 0) {
				LOG_INFO("Data stopped before end. Ignoring shard " + m_shard_id);
				m_cache.clear();
				break;
			}

			total_read_data += read_len;

			size_t num_records = read_len / sizeof(data_record);
			for (size_t i = 0; i < num_records; i++) {
				while (num_records_for_key == 0 && key_id < num_keys) {
					key_id++;
					num_records_for_key = lens[key_id] / sizeof(data_record);
				}

				if (num_records_for_key > 0) {

					const data_record *record = (data_record *)&m_buffer[i * sizeof(data_record)];
					
					m_cache[keys[key_id]].push_back(*record);

					num_records_for_key--;
				}
			}
		}

		return true;
	}

	template<typename data_record>
	void full_text_shard_builder<data_record>::save_file() {

		std::ofstream writer(target_filename(), std::ios::binary | std::ios::trunc);
		if (!writer.is_open()) {
			throw LOG_ERROR_EXCEPTION("Could not open full text shard. Error: " + std::string(strerror(errno)));
		}

		std::ofstream key_writer(key_filename(), std::ios::binary | std::ios::trunc);
		if (!key_writer.is_open()) {
			throw LOG_ERROR_EXCEPTION("Could not open full text shard. Error: " + std::string(strerror(errno)));
		}

		reset_key_file(key_writer);

		std::unordered_map<uint64_t, std::vector<uint64_t>> pages;
		for (auto &iter : m_cache) {
			pages[iter.first % Config::shard_hash_table_size].push_back(iter.first);
		}

		for (const auto &iter : pages) {
			const size_t page_pos = write_page(writer, iter.second);
			writer.flush();
			write_key(key_writer, iter.first, page_pos);
		}

		/*std::sort(keys.begin(), keys.end(), [](const uint64_t a, const uint64_t b) {
			return a < b;
		});

		

		m_cache.clear();
		*/
	}

	template<typename data_record>
	void full_text_shard_builder<data_record>::write_key(std::ofstream &key_writer, uint64_t key, size_t page_pos) {
		assert(key < Config::shard_hash_table_size);
		key_writer.seekp(key * sizeof(uint64_t));
		key_writer.write((char *)&page_pos, sizeof(size_t));
	}

	/*
	 * Writes the page with keys, appending it to the file stream writer. Takes data from m_cache.
	 * */
	template<typename data_record>
	size_t full_text_shard_builder<data_record>::write_page(std::ofstream &writer, const std::vector<uint64_t> &keys) {

		const size_t page_pos = writer.tellp();

		size_t num_keys = keys.size();

		writer.write((char *)&num_keys, 8);
		writer.write((char *)keys.data(), keys.size() * 8);

		std::vector<size_t> v_pos;
		std::vector<size_t> v_len;
		std::vector<size_t> v_tot;

		size_t pos = 0;
		for (uint64_t key : keys) {

			// Store position and length
			size_t len = m_cache[key].size() * sizeof(data_record);
			
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
			writer.write((char *)m_cache[key].data(), sizeof(data_record) * m_cache[key].size());
		}

		return page_pos;
	}

	template<typename data_record>
	void full_text_shard_builder<data_record>::reset_key_file(std::ofstream &key_writer) {
		key_writer.seekp(0);
		uint64_t data = SIZE_MAX;
		for (size_t i = 0; i < Config::shard_hash_table_size; i++) {
			key_writer.write((char *)&data, sizeof(uint64_t));
		}
	}

	template<typename data_record>
	std::string full_text_shard_builder<data_record>::mountpoint() const {
		return std::to_string(m_shard_id % 8);
	}

	template<typename data_record>
	std::string full_text_shard_builder<data_record>::cache_filename() const {
		return "/mnt/" + mountpoint() + "/output/precache_" + m_db_name + "_" + std::to_string(m_shard_id) + ".cache";
	}

	template<typename data_record>
	std::string full_text_shard_builder<data_record>::key_cache_filename() const {
		return "/mnt/" + mountpoint() + "/output/precache_" + m_db_name + "_" + std::to_string(m_shard_id) +".keys";
	}

	template<typename data_record>
	std::string full_text_shard_builder<data_record>::key_filename() const {
		return "/mnt/" + mountpoint() + "/full_text/fti_" + m_db_name + "_" + std::to_string(m_shard_id) + ".keys";
	}

	template<typename data_record>
	std::string full_text_shard_builder<data_record>::target_filename() const {
		return "/mnt/" + mountpoint() + "/full_text/fti_" + m_db_name + "_" + std::to_string(m_shard_id) + ".idx";
	}

	/*
		Deletes ALL data from this shard.
	*/
	template<typename data_record>
	void full_text_shard_builder<data_record>::truncate() {

		truncate_cache_files();

		std::ofstream target_writer(target_filename(), std::ios::trunc);
		target_writer.close();
	}

	/*
		Deletes all data from caches.
	*/
	template<typename data_record>
	void full_text_shard_builder<data_record>::truncate_cache_files() {

		m_cache.clear();

		std::ofstream writer(cache_filename(), std::ios::trunc);
		writer.close();

		std::ofstream key_writer(key_cache_filename(), std::ios::trunc);
		key_writer.close();
	}

	template<typename data_record>
	size_t full_text_shard_builder<data_record>::disk_size() const {

		std::ifstream reader(cache_filename(), std::ios::binary);
		if (!reader.is_open()) {
			throw LOG_ERROR_EXCEPTION("Could not open full text shard (" + cache_filename() + "). Error: " + std::string(strerror(errno)));
		}

		reader.seekg(0, std::ios::end);
		size_t file_size = reader.tellg();

		return file_size;
	}

	template<typename data_record>
	size_t full_text_shard_builder<data_record>::cache_size() const {
		return m_records.size();
	}

}
