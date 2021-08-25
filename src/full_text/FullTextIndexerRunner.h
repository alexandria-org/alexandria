
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

using namespace std;

class FullTextIndexerRunner {

public:

	FullTextIndexerRunner(const string &db_name, const string &hash_table_name, const string &cc_batch, const SubSystem *sub_system);
	FullTextIndexerRunner(const string &db_name, const string &hash_table_name, const string &cc_batch);
	FullTextIndexerRunner(const string &db_name, const string &hash_table_name, const SubSystem *sub_system);
	~FullTextIndexerRunner();

	void run(size_t partition, size_t max_partitions);
	void run(const vector<string> local_files, size_t partition);
	void run_link();
	void merge();
	void sort();
	void truncate_cache();
	void truncate();

private:

	const SubSystem *m_sub_system;
	const string m_cc_batch;
	const string m_hash_table_name;
	const string m_db_name;

	mutex m_hash_table_mutexes[Config::ht_num_shards];
	mutex m_full_text_mutexes[Config::ft_num_shards];
	mutex m_write_url_to_domain_mutex;

	bool m_run_merge_large;
	bool m_did_allocate_sub_system;

	string run_merge_large_thread();
	string run_index_thread(const vector<string> &warc_paths, int id, size_t partition);
	string run_index_thread_with_local_files(const vector<string> &local_files, int id, size_t partition);
	string run_link_index_thread(const vector<string> &warc_paths, int id);
	string run_merge_thread(size_t shard_id);
	int download_file(const string &bucket, const string &key, stringstream &stream);

};
