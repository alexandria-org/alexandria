
#pragma once

#include <iostream>
#include <mutex>
#include "common/common.h"
#include "system/SubSystem.h"
#include "system/ThreadPool.h"
#include "hash_table/HashTable.h"
#include "full_text/FullTextIndex.h"
#include "LinkIndex.h"

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

//#define LI_NUM_THREADS_INDEXING 128
#define LI_NUM_THREADS_INDEXING 16
#define LI_NUM_THREADS_MERGING 64

using namespace std;

class LinkIndexerRunner {

public:

	LinkIndexerRunner(const string &db_name, const string &cc_batch, const string &fti_name);
	~LinkIndexerRunner();

	void run();
	void merge();
	void sort();
	void upload();
	void truncate();
	void index_stream(ifstream &infile);

private:

	const SubSystem *m_sub_system;
	const string m_cc_batch;
	const string m_db_name;
	const string m_fti_name;
	mutex m_hash_table_mutexes[HT_NUM_SHARDS];
	mutex m_full_text_mutexes[FT_NUM_SHARDS];
	mutex m_link_mutexes[LI_NUM_SHARDS];

	string run_index_thread(const vector<string> &warc_paths, int id);
	string run_link_index_thread(const vector<string> &warc_paths, int id);
	string run_merge_thread(size_t shard_id);
	int download_file(const string &bucket, const string &key, stringstream &stream);

};
