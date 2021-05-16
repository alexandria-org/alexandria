
#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <fstream>

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include "SubSystem.h"
#include "BasicData.h"

using namespace std;

class BasicIndexer {

public:

	BasicIndexer(const SubSystem *sub_system);

	void index(const vector<string> &file_names, int shard);

	static string get_downloaded_file_name(int shard, int id);

protected:

	const SubSystem *m_sub_system;

	void download_file(const string &bucket, const string &key, BasicData &index);

};