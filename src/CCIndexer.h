
#pragma once

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include "BasicUrlData.h"
#include "BasicLinkData.h"

using namespace std;

class CCIndexer {

public:

	CCIndexer(const SubSystem *sub_system);
	~CCIndexer();

	void run(const string &bucket, const string &key, int id, int shard);
	void index(const vector<string> &words, const vector<string> &input_files, int shard);
	void sorter(const vector<string> &words, int shard);

private:

	const SubSystem *m_sub_system;

	BasicUrlData m_url_data;
	BasicLinkData m_link_data;

	void download_file(const string &bucket, const string &key, BasicData &index);

};
