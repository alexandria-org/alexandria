
#pragma once

#include <iostream>
#include <vector>

#include "abstract/TextBase.h"
#include "FullTextBucket.h"
#include "FullTextResult.h"

using namespace std;

#define FT_NUM_BUCKETS 8
#define FT_NUM_SHARDS 2048

class FullTextBucket;

class FullTextIndex : public TextBase {

public:
	FullTextIndex(const string &name);
	~FullTextIndex();

	void wait_for_start();

	vector<FullTextResult> search_word(const string &word);
	vector<FullTextResult> search_phrase(const string &phrase);

	size_t disk_size() const;

	// Testable private functions.
	vector<size_t> value_intersection(const map<size_t, vector<uint64_t>> &values_map,
		size_t &shortest_vector_position) const;

private:

	string m_db_name;
	hash<string> m_hasher;

	vector<FullTextBucket *> m_buckets;

	vector<size_t> shard_ids_for_bucket(size_t bucket_id);
	FullTextBucket *bucket_for_hash(size_t hash);
	void sort_results(vector<FullTextResult> &results);

};
