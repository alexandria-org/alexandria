
#pragma once

#include <iostream>
#include <mutex>
#include "common/common.h"
#include "system/SubSystem.h"
#include "system/ThreadPool.h"
#include "hash_table/HashTable.h"
#include "full_text/FullTextIndex.h"
#include "FullTextRecord.h"

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

//#define FT_NUM_THREADS_INDEXING 128

using namespace std;

class FullTextIndexerRunner {

public:

	FullTextIndexerRunner(const string &db_name, const string &hash_table_name, const string &cc_batch, const SubSystem *sub_system);
	~FullTextIndexerRunner();

	void run(size_t partition, size_t max_partitions);
	void run_link();
	void merge();
	void sort();
	void index_text(const string &text);
	void index_text(const string &key, const string &text, float score);
	void index_warc_path(const string warc_path);
	void index_stream(ifstream &infile);
	void truncate();

private:

	const SubSystem *m_sub_system;
	const string m_cc_batch;
	const string m_hash_table_name;
	const string m_db_name;

	mutex m_hash_table_mutexes[HT_NUM_SHARDS];
	mutex m_full_text_mutexes[FT_NUM_SHARDS];
	mutex m_write_url_to_domain_mutex;

	bool m_run_merge_large;

	string run_merge_large_thread();
	string run_index_thread(const vector<string> &warc_paths, int id);
	string run_link_index_thread(const vector<string> &warc_paths, int id);
	string run_merge_thread(size_t shard_id);
	int download_file(const string &bucket, const string &key, stringstream &stream);

};
