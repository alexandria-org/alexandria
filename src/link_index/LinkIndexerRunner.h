
#pragma once

#include <iostream>
#include <mutex>
#include "common/common.h"
#include "system/SubSystem.h"
#include "system/ThreadPool.h"
#include "hash_table/HashTable.h"
#include "full_text/FullTextIndex.h"
#include "full_text/FullTextIndexer.h"
#include "full_text/UrlToDomain.h"
#include "LinkIndex.h"
#include "LinkFullTextRecord.h"
#include "full_text/FullTextRecord.h"

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

using namespace std;

class LinkIndexerRunner {

public:

	LinkIndexerRunner(const string &db_name, const string &hash_table_name, const string &cc_batch, const SubSystem *sub_system,
		UrlToDomain *url_to_domain);
	~LinkIndexerRunner();

	void run(size_t partition, size_t max_partitions);
	void merge();
	void merge_adjustments();
	void sort();
	void upload();
	void truncate();
	void index_stream(ifstream &infile);

private:

	const SubSystem *m_sub_system;
	const string m_cc_batch;
	const string m_db_name;
	const string m_hash_table_name;
	mutex m_hash_table_mutexes[HT_NUM_SHARDS];
	mutex m_full_text_mutexes[FT_NUM_SHARDS];
	mutex m_link_mutexes[FT_NUM_SHARDS];

	UrlToDomain *m_url_to_domain;

	string run_index_thread(const vector<string> &warc_paths, int id);
	string run_link_index_thread(const vector<string> &warc_paths, int id);
	string run_merge_thread(size_t shard_id);
	string run_merge_adjustments_thread(const FullTextIndexer *indexer, size_t shard_id);

};
