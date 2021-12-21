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

template<typename DataRecord> class FullTextIndex;

#include <iostream>
#include <vector>

#include "config.h"
#include "parser/URL.h"
#include "system/SubSystem.h"
#include "system/ThreadPool.h"
#include "FullTextRecord.h"
#include "FullTextShard.h"
#include "SearchMetric.h"
#include "text/Text.h"

#include "system/Logger.h"

template<typename DataRecord>
class FullTextIndex {

public:
	FullTextIndex(const std::string &name, size_t partition);
	~FullTextIndex();

	size_t disk_size() const;

	const std::vector<FullTextShard<DataRecord> *> &shards() { return m_shards; };
	const std::vector<FullTextShard<DataRecord> *> *shard_ptr() { return &m_shards; };

private:

	std::string m_db_name;
	size_t m_partition;
	std::vector<FullTextShard<DataRecord> *> m_shards;

};

template<typename DataRecord>
FullTextIndex<DataRecord>::FullTextIndex(const std::string &db_name, size_t partition)
: m_db_name(db_name), m_partition(partition)
{
	for (size_t shard_id = 0; shard_id < Config::ft_num_shards; shard_id++) {
		m_shards.push_back(new FullTextShard<DataRecord>(m_db_name, shard_id, partition));
	}
}

template<typename DataRecord>
FullTextIndex<DataRecord>::~FullTextIndex() {
	for (FullTextShard<DataRecord> *shard : m_shards) {
		delete shard;
	}
}

template<typename DataRecord>
size_t FullTextIndex<DataRecord>::disk_size() const {
	size_t size = 0;
	for (auto shard : m_shards) {
		size += shard->disk_size();
	}
	return size;
}

