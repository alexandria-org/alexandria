
#pragma once

#include <iostream>
#include <map>
#include <cstdint>
#include "parser/URL.h"
#include "text/Text.h"
#include "FullTextRecord.h"
#include "FullTextIndex.h"

using namespace std;

namespace FullText {

	void truncate_url_to_domain(const string &index_name);
	void truncate_index(const string &index_name, size_t partitions);

	map<uint64_t, float> tsv_data_to_scores(const string &tsv_data, const SubSystem *sub_system);
	void add_words_to_word_map(const vector<string> &words, float score, map<uint64_t, float> &word_map);

	vector<string> make_partition_from_files(const vector<string> &files, size_t partition, size_t max_partitions);

	vector<FullTextRecord> search_index_array(vector<FullTextIndex<FullTextRecord> *> index_array, const vector<LinkFullTextRecord> &links,
		const string &query, size_t limit, struct SearchMetric &metric);

	vector<LinkFullTextRecord> search_link_array(vector<FullTextIndex<LinkFullTextRecord> *> index_array, const string &query, size_t limit,
		struct SearchMetric &metric);

	/*
		Add scores for the given links to the result set. The links are assumed to be ordered by link.m_target_hash ascending.
	*/
	void apply_link_scores(const vector<LinkFullTextRecord> &links, vector<FullTextRecord> &results, struct SearchMetric &metric);

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
