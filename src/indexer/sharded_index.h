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
#include "algorithm/top_k.h"
#include "utils/thread_pool.hpp"
#include "config.h"
#include <mutex>

namespace indexer {

	template<typename data_record>
	class sharded_index {

	public:

		sharded_index(const std::string &db_name, size_t num_shards);
		sharded_index(const std::string &db_name, size_t num_shards, size_t hash_table_size);
		~sharded_index();

		/* 
		 * Find single key
		 * Returns vector with records in storage_order.
		 * */
		std::vector<data_record> find(uint64_t key) const;

		/* 
		 * Find single key
		 * Returns bitmap of internal ids.
		 * */
		roaring::Roaring find_bitmap(uint64_t key) const;

		/*
		 * Find intersection of multiple keys
		 * Returns vector with records in storage order.
		 * */
		std::vector<data_record> find_intersection(const std::vector<uint64_t> &keys) const;

		/*
		 * Find intersection of multiple keys applying lambda function score_mod to the scores before.
		 * Returns n records with highest score.
		 * score_mod is applied in storage_order of data_record.
		 * */
		std::vector<data_record> find_top(size_t &total_num_results, const std::vector<uint64_t> &keys, size_t n, 
				std::function<float(const data_record &)> score_mod = [](const data_record &) { return 0.0f; }) const;

		/*
		 * Overload without total_num_results
		 * */
		std::vector<data_record> find_top(const std::vector<uint64_t> &keys, size_t n, 
				std::function<float(const data_record &)> score_mod = [](const data_record &) { return 0.0f; }) const;

		/*
		 * Find intersection of multiple keys and run group by, the groups will be determined by the
		 * data_record::storage_equal predicate and 'score_formula' will be applied to m_score before summing.
		 * Returns vector with grouped records.
		 * */
		std::vector<data_record> find_group_by(const std::vector<uint64_t> &keys,
				std::function<float(float)> score_formula, std::vector<size_t> &counts) const;

		/*
		 * Calculates a set of keys that has more than the given number of records.
		 * Returns a std::set<uint64_t> with the keys.
		 * This function is slow. Needs to open each shard to retrieve the keys.
		 * */
		std::set<uint64_t> get_keys(size_t with_more_than_records) const;

		/*
		 * Iterates the keys of the index and calls the callback with key and the bitmap for that key.
		 * */
		void for_each(std::function<void(uint64_t key, roaring::Roaring &bitmap)> on_each_key) const;

		/*
		 * Returns the total number of records.
		 * */
		size_t num_records() const { return m_records.size(); }

		/*
		 * Copies all the records from the bitmap into the vector append_to
		 * */
		void get_records_for_bitmap(const roaring::Roaring &bitmap, std::vector<data_record> &append_to) const;

	private:

		std::string m_db_name;
		size_t m_num_shards;
		size_t m_hash_table_size;

		std::vector<data_record> m_records;
		mutable std::vector<float> m_scores;
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
	roaring::Roaring sharded_index<data_record>::find_bitmap(uint64_t key) const {

		const size_t shard_id = key % m_num_shards;
		index<data_record> idx(m_db_name, shard_id, m_hash_table_size);

		return idx.find_bitmap(key);
	}

	template<typename data_record>
	std::vector<data_record> sharded_index<data_record>::find_intersection(const std::vector<uint64_t> &keys) const {

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
	std::vector<data_record> sharded_index<data_record>::find_top(size_t &total_num_results, const std::vector<uint64_t> &keys, size_t n,
			std::function<float(const data_record &)> score_mod) const {

		std::fill(m_scores.begin(), m_scores.end(), 0.0f);

		std::vector<roaring::Roaring> results;
		for (uint64_t key : keys) {

			const size_t shard_id = key % m_num_shards;
			index<data_record> idx(m_db_name, shard_id, m_hash_table_size);
			
			roaring::Roaring res = idx.find_bitmap(key);
			results.emplace_back(std::move(res));
		}

		roaring::Roaring rr = ::algorithm::intersection(results);

		total_num_results = rr.cardinality();

		// Apply score modifications.
		std::vector<uint32_t> ids;
		for (uint32_t internal_id : rr) {
			ids.push_back(internal_id);
			m_scores[internal_id] = m_records[internal_id].m_score * score_mod(m_records[internal_id].m_value);
		}

		auto ordered = [this](const uint32_t &a, const uint32_t &b) {
			return m_scores[a] < m_scores[b];
		};

		std::vector<uint32_t> top_ids = ::algorithm::top_k<uint32_t>(ids, n, ordered);

		std::vector<data_record> ret;
		for (uint32_t internal_id : top_ids) {
			ret.push_back(m_records[internal_id]);
			ret.back().m_score = m_scores[internal_id];
		}

		sort(ret.begin(), ret.end(), typename data_record::score_order());

		return ret;
	}

	template<typename data_record>
	std::vector<data_record> sharded_index<data_record>::find_top(const std::vector<uint64_t> &keys, size_t n,
			std::function<float(const data_record &)> score_mod) const {

		size_t total_num_results = 0;
		return find_top(total_num_results, keys, n, score_mod);
	}

	template<typename data_record>
	std::vector<data_record> sharded_index<data_record>::find_group_by(const std::vector<uint64_t> &keys,
			std::function<float(float)> score_formula, std::vector<size_t> &counts) const {

		std::vector<roaring::Roaring> results;
		for (uint64_t key : keys) {

			const size_t shard_id = key % m_num_shards;
			index<data_record> idx(m_db_name, shard_id, m_hash_table_size);
			
			roaring::Roaring res = idx.find_bitmap(key);
			results.emplace_back(std::move(res));
		}

		roaring::Roaring rr = ::algorithm::intersection(results);

		// Group by.
		std::vector<data_record> ret;
		for (uint32_t internal_id : rr) {
			if (internal_id >= m_records.size()) {
				std::cout << "internal_id: " << internal_id << " >= " << m_records.size() << std::endl;
				continue;
			}
			if (ret.size() && ret.back().storage_equal(m_records[internal_id])) {
				ret.back().m_score += score_formula(m_records[internal_id].m_score);
				counts.back()++;
			} else {
				ret.emplace_back(m_records[internal_id]);
				ret.back().m_score = score_formula(ret.back().m_score);
				counts.push_back(1);
			}
		}

		return ret;
	}

	template<typename data_record>
	std::set<uint64_t> sharded_index<data_record>::get_keys(size_t with_more_than_records) const {

		utils::thread_pool pool(32);
		std::mutex lock;
		std::set<uint64_t> all_keys;
		for (size_t shard_id = 0; shard_id < m_num_shards; shard_id++) {

			pool.enqueue([this, shard_id, with_more_than_records, &all_keys, &lock]() {
				index<data_record> idx(m_db_name, shard_id, m_hash_table_size);
				std::set<uint64_t> keys_for_shard = idx.get_keys(with_more_than_records);

				lock.lock();
				all_keys.insert(keys_for_shard.begin(), keys_for_shard.end());
				lock.unlock();
			});
		}

		pool.run_all();

		return all_keys;

	}

	template<typename data_record>
	void sharded_index<data_record>::for_each(std::function<void(uint64_t key, roaring::Roaring &bitmap)> on_each_key) const {
	
		utils::thread_pool pool(32);
		for (size_t shard_id = 0; shard_id < m_num_shards; shard_id++) {

			pool.enqueue([this, shard_id, &on_each_key]() {
				index<data_record> idx(m_db_name, shard_id, m_hash_table_size);
				idx.for_each(on_each_key);
			});
		}

		pool.run_all();
	}

	/*
	 * Copies all the records from the bitmap into the vector iterator "append_to"
	 * */
	template<typename data_record>
	void sharded_index<data_record>::get_records_for_bitmap(const roaring::Roaring &bitmap, std::vector<data_record> &append_to) const {
		for (uint32_t internal_id : bitmap) {
			append_to.emplace_back(m_records[internal_id]);
		}
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
				m_scores.push_back(0.0f);
			}
		}
	}

	template<typename data_record>
	std::string sharded_index<data_record>::filename() const {
		// This file will contain meta data on the index. For example the hyper log log document counter.
		return config::data_path() + "/0/full_text/" + m_db_name + ".meta";
	}

}
