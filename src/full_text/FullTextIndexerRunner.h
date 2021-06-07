
#pragma once

#include <iostream>
#include <mutex>
#include "common/common.h"
#include "system/SubSystem.h"
#include "system/ThreadPool.h"
#include "hash_table/HashTable.h"

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#define FT_NUM_THREADS_INDEXING 8

using namespace std;

class FullTextIndexerRunner {

public:

	FullTextIndexerRunner(const string &cc_batch);
	~FullTextIndexerRunner();

	void run();

private:

	const SubSystem *m_sub_system;
	const string m_cc_batch;
	mutex m_write_mutex;
	mutex m_hash_table_mutexes[HT_NUM_SHARDS];

	string run_index_thread(const vector<string> &warc_paths, int id);
	int download_file(const string &bucket, const string &key, stringstream &stream);

};
