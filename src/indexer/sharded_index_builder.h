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

#include "index_builder.h"
#include "algorithm/hyper_log_log.h"

#include <numeric>

namespace indexer {

	template<typename data_record>
	class sharded_index_builder {
	private:
		// Non copyable
		sharded_index_builder(const sharded_index_builder &);
		sharded_index_builder& operator=(const sharded_index_builder &);

	public:

		sharded_index_builder(const std::string &db_name, size_t num_shards);
		~sharded_index_builder();

		void add(uint64_t key, const data_record &record);
		
		void append();
		void merge();
		void merge_one(size_t id);
		void optimize();

		/*
			This function calculate scores. Should run after a merge.
		*/
		void calculate_scores(algorithm algo);

		size_t num_documents() const { return m_document_counter.count(); }
		size_t document_size(uint64_t document_id) { return m_document_sizes[document_id]; }

		void truncate();
		void truncate_cache_files();
		void create_directories();

	private:

		std::mutex m_lock;
		std::string m_db_name;
		std::vector<std::shared_ptr<index_builder<data_record>>> m_shards;
		::algorithm::hyper_log_log m_document_counter;
		std::map<uint64_t, size_t> m_document_sizes;
		float m_avg_document_size = 0.0f;

		std::vector<data_record> m_records;
		std::map<uint64_t, uint32_t> m_record_id_map;

		void read_meta();
		void write_meta();
		std::string filename() const;
		bool needs_optimization() const;
		void sort_records();

	};

	template<typename data_record>
	sharded_index_builder<data_record>::sharded_index_builder(const std::string &db_name, size_t num_shards) {

		std::function<uint32_t(const data_record &)> rec_to_id = [this](const data_record &record) {
			std::lock_guard guard(m_lock);
			if (m_record_id_map.count(record.m_value) == 0) {
				m_record_id_map[record.m_value] = m_records.size();
				m_records.push_back(record);
			}
			return m_record_id_map[record.m_value];
		};

		m_db_name = db_name;
		for (size_t shard_id = 0; shard_id < num_shards; shard_id++) {
			m_shards.push_back(std::make_shared<index_builder<data_record>>(db_name, shard_id, rec_to_id));
		}
		create_directories();
		read_meta();
	}

	template<typename data_record>
	sharded_index_builder<data_record>::~sharded_index_builder() {
		write_meta();
	}

	template<typename data_record>
	void sharded_index_builder<data_record>::add(uint64_t key, const data_record &record) {
		m_shards[key % m_shards.size()]->add(key, record);

		/*m_document_counter.insert(record.m_value);
		m_document_sizes[record.m_value]++; // Raw non unique document size.
		*/
	}

	template<typename data_record>
	void sharded_index_builder<data_record>::append() {
		for (auto &shard : m_shards) {
			shard->append();
		}
	}

	template<typename data_record>
	void sharded_index_builder<data_record>::merge() {
		for (auto &shard : m_shards) {
			shard->merge();
		}
	}

	template<typename data_record>
	void sharded_index_builder<data_record>::merge_one(size_t id) {
		m_shards[id]->merge();
	}

	template<typename data_record>
	void sharded_index_builder<data_record>::optimize() {
		if (needs_optimization()) {
			sort_records();
		}
	}

	template<typename data_record>
	void sharded_index_builder<data_record>::calculate_scores(algorithm algo) {

		/*const size_t num_docs = num_documents();
		score_builder score(num_docs, &m_document_sizes);
		
		for (auto &shard : m_shards) {
			shard->calculate_scores(algo, score);
		}*/
	}

	template<typename data_record>
	void sharded_index_builder<data_record>::truncate() {
		for (auto &shard : m_shards) {
			shard->truncate();
		}
		std::ofstream meta_file(filename(), std::ios::trunc);
		m_records = std::vector<data_record>{};
		m_record_id_map = std::map<uint64_t, uint32_t>{};
	}

	template<typename data_record>
	void sharded_index_builder<data_record>::truncate_cache_files() {
		for (auto &shard : m_shards) {
			shard->truncate_cache_files();
		}
	}

	template<typename data_record>
	void sharded_index_builder<data_record>::create_directories() {
		for (auto &shard : m_shards) {
			shard->create_directories();
		}
	}

	template<typename data_record>
	void sharded_index_builder<data_record>::read_meta() {
		std::ifstream meta_file(filename(), std::ios::binary);

		if (meta_file.is_open()) {

			// Read records.
			size_t num_records;
			meta_file.read((char *)(&num_records), sizeof(size_t));
			if (meta_file.eof()) return;
			for (size_t i = 0; i < num_records; i++) {
				data_record rec;
				meta_file.read((char *)(&rec), sizeof(data_record));

				m_record_id_map[rec.m_value] = m_records.size();
				m_records.push_back(rec);
			}

		}
	}

	template<typename data_record>
	void sharded_index_builder<data_record>::write_meta() {
		std::ofstream meta_file(filename(), std::ios::binary | std::ios::trunc);

		if (meta_file.is_open()) {

			// Write records.
			const size_t num_records = m_records.size();
			meta_file.write((char *)(&num_records), sizeof(size_t));
			for (const data_record &record : m_records) {
				meta_file.write((char *)(&record), sizeof(data_record));
			}
		}
	}

	template<typename data_record>
	std::string sharded_index_builder<data_record>::filename() const {
		// This file will contain meta data on the index. For example the hyper log log document counter.
		return "/mnt/0/full_text/" + m_db_name + ".meta";
	}

	template<typename data_record>
	bool sharded_index_builder<data_record>::needs_optimization() const {
		// Just check if the records are sorted by storage order.
		if (m_records.size() <= 1) return false;
		
		typename data_record::storage_order ordered;
		for (size_t i = 0; i < m_records.size() - 1; i++) {
			if (!ordered(m_records[i], m_records[i + 1])) {
				return true;
			}
		}
		return false;
	}

	template<typename data_record>
	void sharded_index_builder<data_record>::sort_records() {
		std::vector<uint32_t> permutation(m_records.size());
		std::iota(permutation.begin(), permutation.end(), 0);

		typename data_record::storage_order ordered;

		std::sort(permutation.begin(), permutation.end(), [this, &ordered](const size_t &a, const size_t &b) {
			return ordered(m_records[a], m_records[b]);
		});

		for (auto &shard : m_shards) {
			shard->transform([&permutation](uint32_t v) {
				return permutation[v];
			});
		}

		// Reorder the records and save...
		sort(m_records.begin(), m_records.end(), ordered);
	}

}
