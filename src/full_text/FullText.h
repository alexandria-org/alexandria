
#pragma once

#include <iostream>
#include <map>
#include <cstdint>
#include "parser/URL.h"
#include "text/Text.h"
#include "UrlToDomain.h"
#include "FullTextRecord.h"
#include "FullTextIndex.h"

using namespace std;

namespace FullText {

	size_t num_shards();

	void truncate_url_to_domain(const string &index_name);
	void truncate_index(const string &index_name, size_t partitions);

	map<uint64_t, float> tsv_data_to_scores(const string &tsv_data, const SubSystem *sub_system);
	void add_words_to_word_map(const vector<string> &words, float score, map<uint64_t, float> &word_map);

	vector<string> make_partition_from_files(const vector<string> &files, size_t partition, size_t max_partitions);

	vector<string> download_batch(const string &batch, size_t limit, size_t offset);
	void index_all_batches(const string &db_name, const string &hash_table_name);
	void index_batch(const string &db_name, const string &hash_table_name, const string &batch, const SubSystem *sub_system);
	void index_single_batch(const string &db_name, const string &domain_db_name, const string &batch);
	void index_all_link_batches(const string &db_name, const string &domain_db_name, const string &hash_table_name,
			const string &domain_hash_table_name);
	void index_link_batch(const string &db_name, const string &domain_db_name, const string &hash_table_name, const string &domain_hash_table_name,
		const string &batch, const SubSystem *sub_system, UrlToDomain *url_to_domain);
	void index_single_link_batch(const string &db_name, const string &domain_db_name, const string &hash_table_name,
		const string &domain_hash_table_name, const string &batch);
	bool should_index_url(const URL &url, size_t partition);
	bool should_index_hash(size_t hash, size_t partition);

	template<typename DataRecord>
	vector<FullTextIndex<DataRecord> *> create_index_array(const string &db_name, size_t partitions) {

		vector<FullTextIndex<DataRecord> *> index_array;
		for (size_t partition = 0; partition < partitions; partition++) {
			index_array.push_back(new FullTextIndex<DataRecord>(db_name + "_" + to_string(partition)));
		}

		return index_array;
	}

	template<typename DataRecord>
	void delete_index_array(vector<FullTextIndex<DataRecord> *> &index_array) {

		for (FullTextIndex<DataRecord> *index : index_array) {
			delete index;
		}

	}

}
