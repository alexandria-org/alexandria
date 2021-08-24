
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
#include "FullTextResultSet.h"
#include "SearchMetric.h"
#include "link_index/LinkFullTextRecord.h"
#include "text/Text.h"

#include "system/Logger.h"

using namespace std;

template<typename DataRecord>
class FullTextIndex {

public:
	FullTextIndex(const string &name);
	~FullTextIndex();

	size_t disk_size() const;

	const vector<FullTextShard<DataRecord> *> &shards() { return m_shards; };

private:

	string m_db_name;
	vector<FullTextShard<DataRecord> *> m_shards;

};

template<typename DataRecord>
FullTextIndex<DataRecord>::FullTextIndex(const string &db_name)
: m_db_name(db_name)
{
	for (size_t shard_id = 0; shard_id < Config::ft_num_shards; shard_id++) {
		m_shards.push_back(new FullTextShard<DataRecord>(m_db_name, shard_id));
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

