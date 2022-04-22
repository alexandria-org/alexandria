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
#include "algorithm/hyper_log_log.h"

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

	private:

		std::mutex m_lock;
		std::string m_db_name;
		std::vector<std::shared_ptr<index_type<data_record>>> m_shards;

		::algorithm::hyper_log_log m_document_counter;
		std::map<uint64_t, size_t> m_document_sizes;
		float m_avg_document_size = 0.0f;

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

		m_document_counter.insert(record.m_value);
		m_document_sizes[record.m_value]++; // Raw non unique document size.
	}

	template<template<typename> typename index_type, typename data_record>
	void sharded_builder<index_type, data_record>::append() {
		for (auto &shard : m_shards) {
			shard->append();
		}
	}

	template<template<typename> typename index_type, typename data_record>
	void sharded_builder<index_type, data_record>::merge() {
		for (auto &shard : m_shards) {
			shard->merge();
		}
	}

	template<template<typename> typename index_type, typename data_record>
	void sharded_builder<index_type, data_record>::truncate() {
		for (auto &shard : m_shards) {
			shard->truncate();
		}
		std::ofstream meta_file(filename(), std::ios::trunc);
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
	void sharded_builder<index_type, data_record>::read_meta() {
		std::ifstream meta_file(filename(), std::ios::binary);

		if (meta_file.is_open()) {
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
		return "/mnt/0/full_text/" + m_db_name + ".meta";
	}

}
