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

#include "index.h"
#include "algorithm/intersection.h"
#include "config.h"

namespace indexer {

	template<typename data_record>
	class sharded_index {

	public:

		sharded_index(const std::string &db_name, size_t num_shards);
		sharded_index(const std::string &db_name, size_t num_shards, size_t hash_table_size);
		~sharded_index();

		std::vector<data_record> find(uint64_t key) const;
		std::vector<data_record> find(const std::vector<uint64_t> &keys) const;

	private:

		std::string m_db_name;
		size_t m_num_shards;
		size_t m_hash_table_size;

		std::vector<data_record> m_records;
		std::map<uint64_t, uint32_t> m_record_id_map;

		void read_meta();
		std::string filename() const;

	};

	template<typename data_record>
	sharded_index<data_record>::sharded_index(const std::string &db_name, size_t num_shards)
	: m_db_name(db_name), m_num_shards(num_shards), m_hash_table_size(config::shard_hash_table_size)
	{
		read_meta();
	}

	template<typename data_record>
	sharded_index<data_record>::sharded_index(const std::string &db_name, size_t num_shards, size_t hash_table_size)
	: m_db_name(db_name), m_num_shards(num_shards), m_hash_table_size(hash_table_size)
	{
		read_meta();
	}

	template<typename data_record>
	sharded_index<data_record>::~sharded_index() {
	}

	template<typename data_record>
	std::vector<data_record> sharded_index<data_record>::find(uint64_t key) const {

		const size_t shard_id = key % m_num_shards;
		index<data_record> idx(m_db_name, shard_id, m_hash_table_size);

		roaring::Roaring rr = idx.find_bitmap(key);

		std::function<data_record(uint32_t id)> id_to_rec = [this](uint32_t id) {
			return m_records[id];
		};

		std::vector<data_record> ret;
		for (uint32_t internal_id : rr) {
			ret.emplace_back(id_to_rec(internal_id));
		}

		return ret;
	}

	template<typename data_record>
	std::vector<data_record> sharded_index<data_record>::find(const std::vector<uint64_t> &keys) const {

		std::vector<roaring::Roaring> results;
		for (uint64_t key : keys) {

			const size_t shard_id = key % m_num_shards;
			index<data_record> idx(m_db_name, shard_id, m_hash_table_size);
			
			roaring::Roaring res = idx.find_bitmap(key);
			results.emplace_back(std::move(res));
		}

		roaring::Roaring rr = ::algorithm::intersection(results);

		std::function<data_record(uint32_t id)> id_to_rec = [this](uint32_t id) {
			return m_records[id];
		};

		std::vector<data_record> ret;
		for (uint32_t internal_id : rr) {
			ret.emplace_back(id_to_rec(internal_id));
		}

		return ret;
	}

	template<typename data_record>
	void sharded_index<data_record>::read_meta() {
		std::ifstream meta_file(filename(), std::ios::binary);

		if (meta_file.is_open()) {

			// Read records.
			size_t num_records;
			meta_file.read((char *)(&num_records), sizeof(size_t));
			for (size_t i = 0; i < num_records; i++) {
				data_record rec;
				meta_file.read((char *)(&rec), sizeof(data_record));

				m_record_id_map[rec.m_value] = m_records.size();
				m_records.push_back(rec);
			}
		}
	}

	template<typename data_record>
	std::string sharded_index<data_record>::filename() const {
		// This file will contain meta data on the index. For example the hyper log log document counter.
		return "/mnt/0/full_text/" + m_db_name + ".meta";
	}

}
