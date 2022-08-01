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
#include <fstream>
#include "algorithm/hyper_log_log.h"
#include "utils/thread_pool.hpp"
#include "debug.h"
#include "config.h"

namespace indexer {

	template<template<typename> typename index_type, typename data_record>
	class sharded_builder {
	private:
		// Non copyable
		sharded_builder(const sharded_builder &);
		sharded_builder& operator=(const sharded_builder &);

	public:

		sharded_builder(const std::string &db_name, size_t num_shards);
		~sharded_builder();

		void add(uint64_t key, const data_record &record);
		
		void append();
		void merge();

		void truncate();
		void truncate_cache_files();
		void create_directories();

		size_t document_count() const { return m_document_counter.count(); }
		size_t document_size(uint64_t value) { return m_document_sizes[value]; }

		void calculate_scores();
		void sort_by_scores();

	private:

		std::mutex m_lock;
		std::string m_db_name;
		std::vector<std::shared_ptr<index_type<data_record>>> m_shards;

		::algorithm::hyper_log_log m_document_counter;
		std::map<uint64_t, size_t> m_document_sizes;
		float m_avg_document_size = 0.0f;
		size_t m_num_added_keys = 0;

		void read_meta();
		void write_meta();
		std::string filename() const;

	};

	template<template<typename> typename index_type, typename data_record>
	sharded_builder<index_type, data_record>::sharded_builder(const std::string &db_name, size_t num_shards) {

		m_db_name = db_name;
		for (size_t shard_id = 0; shard_id < num_shards; shard_id++) {
			m_shards.push_back(std::make_shared<index_type<data_record>>(db_name, shard_id));
		}
		create_directories();
		read_meta();
	}

	template<template<typename> typename index_type, typename data_record>
	sharded_builder<index_type, data_record>::~sharded_builder() {
		write_meta();
	}

	template<template<typename> typename index_type, typename data_record>
	void sharded_builder<index_type, data_record>::add(uint64_t key, const data_record &record) {
		m_shards[key % m_shards.size()]->add(key, record);

		/*m_num_added_keys++;
		m_document_counter.insert(record.m_value);
		m_document_sizes[record.m_value]++;*/ // Raw non unique document size.
	}

	template<template<typename> typename index_type, typename data_record>
	void sharded_builder<index_type, data_record>::append() {
		for (auto &shard : m_shards) {
			shard->append();
		}
	}

	template<template<typename> typename index_type, typename data_record>
	void sharded_builder<index_type, data_record>::merge() {
		utils::thread_pool pool(32);
		for (size_t i = 0; i < m_shards.size(); i++) {
			pool.enqueue([this, i]() {
				try {
					m_shards[i]->merge();
				} catch (...) {
				}
			});
		}

		pool.run_all();
	}

	template<template<typename> typename index_type, typename data_record>
	void sharded_builder<index_type, data_record>::truncate() {
		for (auto &shard : m_shards) {
			shard->truncate();
		}
		std::ofstream meta_file(filename(), std::ios::trunc);

		m_document_counter.reset();
		m_document_sizes.clear();
		m_num_added_keys = 0;
	}

	template<template<typename> typename index_type, typename data_record>
	void sharded_builder<index_type, data_record>::truncate_cache_files() {
		for (auto &shard : m_shards) {
			shard->truncate_cache_files();
		}
	}

	template<template<typename> typename index_type, typename data_record>
	void sharded_builder<index_type, data_record>::create_directories() {
		for (auto &shard : m_shards) {
			shard->create_directories();
		}
	}

	template<template<typename> typename index_type, typename data_record>
	void sharded_builder<index_type, data_record>::calculate_scores() {

		const size_t total_records = m_document_counter.count();
		double average_document_size = 0.0f;
		for (const auto &iter : m_document_sizes) {
			average_document_size += iter.second;
		}
		average_document_size /= m_document_sizes.size();

		const auto tf_idf = [this, total_records](const data_record &rec, size_t num_records) {
			data_record ret = rec;
			float tf = (float)rec.m_count/m_document_sizes[rec.m_value];
			float idf = (float)total_records/num_records;
			ret.m_score = tf*log(idf);
			return ret;
		};

		const auto bm25 = [this, total_records, average_document_size](const data_record &rec, size_t num_records) {

			if (m_document_sizes[rec.m_value] < 1000) {
				data_record ret = rec;
				ret.m_score = 0.0f;
				return ret;
			}

			// https://en.wikipedia.org/wiki/Okapi_BM25
			const double N = total_records; 
			const double n_q = num_records;
			const double idf = log((N - n_q + 0.5)/(n_q + 0.5) + 1.0);

			const double count_d = rec.m_count;
			const double doc_size_d = m_document_sizes[rec.m_value];

			const double f_q = count_d/doc_size_d;
			const double k1 = 1.2;
			const double b = 0.75;
			const double d_card = m_document_sizes[rec.m_value];

			const double score = idf * (f_q * (k1 + 1.0)) / (f_q + k1 * (1.0 - b + b * (d_card / average_document_size)));

			data_record ret = rec;
			ret.m_score = score;
			return ret;
		};

		(void)tf_idf;

		const auto algo = bm25;

		utils::thread_pool pool(32);
		for (size_t i = 0; i < m_shards.size(); i++) {
			pool.enqueue([this, i, algo](){
				m_shards[i]->transform(algo);
			});
		}
		pool.run_all();
	}

	template<template<typename> typename index_type, typename data_record>
	void sharded_builder<index_type, data_record>::sort_by_scores() {

		utils::thread_pool pool(32);
		for (size_t i = 0; i < m_shards.size(); i++) {
			pool.enqueue([this, i](){
				m_shards[i]->sort_by([](const data_record &a, const data_record &b) {
					return a.m_score > b.m_score;
				});
			});
		}
		pool.run_all();
	}

	template<template<typename> typename index_type, typename data_record>
	void sharded_builder<index_type, data_record>::read_meta() {
		std::ifstream meta_file(filename(), std::ios::binary);

		if (meta_file.is_open()) {

			meta_file.read((char *)&m_num_added_keys, sizeof(size_t));

			char *data = m_document_counter.data();
			meta_file.read(data, m_document_counter.data_size());

			size_t num_docs = 0;
			meta_file.read((char *)(&num_docs), sizeof(size_t));
			for (size_t i = 0; i < num_docs; i++) {
				uint64_t doc_id = 0;
				size_t count = 0;
				meta_file.read((char *)(&doc_id), sizeof(uint64_t));
				meta_file.read((char *)(&count), sizeof(size_t));
				m_document_sizes[doc_id] = count;
			}
		}
	}

	template<template<typename> typename index_type, typename data_record>
	void sharded_builder<index_type, data_record>::write_meta() {
		std::ofstream meta_file(filename(), std::ios::binary | std::ios::trunc);

		if (meta_file.is_open()) {

			meta_file.write((char *)&m_num_added_keys, sizeof(size_t));

			char *data = m_document_counter.data();
			meta_file.write(data, m_document_counter.data_size());

			// Write document sizes.
			const size_t num_docs = m_document_sizes.size();
			meta_file.write((char *)(&num_docs), sizeof(size_t));
			for (const auto &iter : m_document_sizes) {
				meta_file.write((char *)(&iter.first), sizeof(uint64_t));
				meta_file.write((char *)(&iter.second), sizeof(size_t));
			}
		}
	}

	template<template<typename> typename index_type, typename data_record>
	std::string sharded_builder<index_type, data_record>::filename() const {
		// This file will contain meta data on the index. For example the hyper log log document counter.
		return config::data_path() + "/0/full_text/" + m_db_name + ".meta";
	}

}
