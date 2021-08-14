
#pragma once

#include "hash_table/HashTable.h"
#include "full_text/FullTextIndex.h"
#include "full_text/FullTextRecord.h"

#include "link_index/LinkFullTextRecord.h"

namespace Api {

	void search(const string &query, HashTable &hash_table, vector<FullTextIndex<FullTextRecord> *> index_array,
		vector<FullTextIndex<LinkFullTextRecord> *> link_index_array, stringstream &response_stream);

	void word_stats(const string &query, vector<FullTextIndex<FullTextRecord> *> index_array,
		vector<FullTextIndex<LinkFullTextRecord> *> link_index_array, size_t index_size, size_t link_index_size, stringstream &response_stream);

}
