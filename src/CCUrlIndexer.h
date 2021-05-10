
#pragma once

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <cmath>

#include "SubSystem.h"
#include "ThreadPool.h"
#include "BasicUrlData.h"
#include "BasicIndexer.h"

using namespace std;

#define CC_NUM_THREADS_DOWNLOADING 128
#define CC_NUM_THREADS_UPLOADING 512
#define CC_NUM_THREADS_INDEXING 32

class CCUrlIndexer : public BasicIndexer {

public:

	CCUrlIndexer(const SubSystem *sub_system);
	~CCUrlIndexer();

	static void run_all();
	static void run_all(size_t limit);

	void download(const string &bucket, const string &key, int id, int shard);
	void sorter(const vector<string> &words, int shard);

private:

	const SubSystem *m_sub_system;

	void find_chunk_positions(const vector<vector<string>> &chunks, const vector<string> &input_files,
		map<size_t, map<string, pair<size_t, size_t>>> &container);

};
