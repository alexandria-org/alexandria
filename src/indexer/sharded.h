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
#include "algorithm/sum_sorted.h"
#include "algorithm/intersection.h"

namespace indexer {

	template<template<typename> typename index_type, typename data_record>
	class sharded {

	public:

		sharded(const std::string &db_name, size_t num_shards);
		sharded(const std::string &db_name, size_t num_shards, size_t hash_table_size);
		~sharded();

		/* 
		 * Find single key
		 * Returns vector with records in storage_order.
		 * */
		std::vector<data_record> find(uint64_t key) const;
		std::vector<data_record> find(uint64_t key, size_t limit) const;

		/*
		 * Find intersection of multiple keys
		 * Returns vector with records in storage order.
		 * */
		std::vector<data_record> find_intersection(const std::vector<uint64_t> &keys) const;

		/*
		 * Find each key in keys and records with same m_value each key only returns top 'limit' number of results.
		 * Returns vector with summed records.
		 * */
		std::vector<data_record> find_sum(const std::vector<uint64_t> &keys, size_t limit) const;

	private:

		std::string m_db_name;
		size_t m_num_shards;
		size_t m_hash_table_size;

		void read_meta();
		std::string filename() const;

	};

	template<template<typename> typename index_type, typename data_record>
	sharded<index_type, data_record>::sharded(const std::string &db_name, size_t num_shards)
	: m_db_name(db_name), m_num_shards(num_shards), m_hash_table_size(config::shard_hash_table_size)
	{
		read_meta();
	}

	template<template<typename> typename index_type, typename data_record>
	sharded<index_type, data_record>::sharded(const std::string &db_name, size_t num_shards, size_t hash_table_size)
	: m_db_name(db_name), m_num_shards(num_shards), m_hash_table_size(hash_table_size)
	{
		read_meta();
	}

	template<template<typename> typename index_type, typename data_record>
	sharded<index_type, data_record>::~sharded() {
	}

	template<template<typename> typename index_type, typename data_record>
	std::vector<data_record> sharded<index_type, data_record>::find(uint64_t key) const {

		const size_t shard_id = key % m_num_shards;
		index_type<data_record> idx(m_db_name, shard_id, m_hash_table_size);

		return idx.find(key);
	}

	template<template<typename> typename index_type, typename data_record>
	std::vector<data_record> sharded<index_type, data_record>::find(uint64_t key, size_t limit) const {

		const size_t shard_id = key % m_num_shards;
		index_type<data_record> idx(m_db_name, shard_id, m_hash_table_size);

		return idx.find(key, limit);
	}

	template<template<typename> typename index_type, typename data_record>
	std::vector<data_record> sharded<index_type, data_record>::find_intersection(const std::vector<uint64_t> &keys) const {

		std::vector<unique_ptr<data_record[]>> results;
		std::vector<size_t> num_results;
		for (uint64_t key : keys) {

			const size_t shard_id = key % m_num_shards;
			index_type<data_record> idx(m_db_name, shard_id, m_hash_table_size);
			
			size_t num_records;
			unique_ptr<data_record[]> res = idx.find_ptr(key, num_records);
			results.emplace_back(std::move(res));
			num_results.push_back(num_records);
		}

		std::vector<data_record> ret = ::algorithm::intersection(results, num_results);

		return ret;
	}

	template<template<typename> typename index_type, typename data_record>
	std::vector<data_record> sharded<index_type, data_record>::find_sum(const std::vector<uint64_t> &keys,
			size_t limit) const {

		std::vector<std::vector<data_record>> results;
		for (uint64_t key : keys) {
			const size_t shard_id = key % m_num_shards;
			index_type<data_record> idx(m_db_name, shard_id, m_hash_table_size);

			std::vector<data_record> res = idx.find(key, limit);

			sort(res.begin(), res.end());

			results.emplace_back(std::move(res));
		}

		// Sum equal elements.
		return ::algorithm::sum_sorted<data_record>(results, [](data_record &a, const data_record &b) {
			a.m_score += b.m_score;
		});

	}

	template<template<typename> typename index_type, typename data_record>
	void sharded<index_type, data_record>::read_meta() {
		std::ifstream meta_file(filename(), std::ios::binary);

		if (meta_file.is_open()) {

		}
	}

	template<template<typename> typename index_type, typename data_record>
	std::string sharded<index_type, data_record>::filename() const {
		// This file will contain meta data on the index. For example the hyper log log document counter.
		return "/mnt/0/full_text/" + m_db_name + ".meta";
	}

}
