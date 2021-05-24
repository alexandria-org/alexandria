
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
#include "BasicLinkData.h"
#include "BasicIndexer.h"

using namespace std;

class CCLinkIndex : public BasicIndexer {

public:

	CCLinkIndex(const SubSystem *sub_system);

	void download(const string &bucket, const string &file, int shard, int id);
	void sorter(const vector<string> &words);
	void upload(const string &cc_batch, const string &word, size_t retries);

private:

	BasicLinkData m_link_data;

};
