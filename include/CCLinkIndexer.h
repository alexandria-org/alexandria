
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

class CCLinkIndexer : public BasicIndexer {

public:

	CCLinkIndexer(const SubSystem *sub_system);
	~CCLinkIndexer();

	string download(const string &bucket, const string &key, int id, int shard);
	void sorter(const vector<string> &words);

private:

	const SubSystem *m_sub_system;

	BasicLinkData m_link_data;

	void download_file(const string &bucket, const string &key, BasicData &index);

};
