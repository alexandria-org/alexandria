
#pragma once

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>

#include <boost/filesystem.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include "common.h"
#include "TsvFile.h"

using namespace std;
using namespace boost::iostreams;

#define TSV_FILE_BUCKET "alexandria-database"
#define TSV_FILE_BUCKET_DEV "alexandria-database-dev"
#define TSV_FILE_DESTINATION "/mnt/0"

class TsvFileS3 : public TsvFile {

public:

	TsvFileS3(const Aws::S3::S3Client &s3_client, const string &file_name);
	TsvFileS3(const Aws::S3::S3Client &s3_client, const string &bucket, const string &file_name);
	~TsvFileS3();

	string get_path() const;

private:
	Aws::S3::S3Client m_s3_client;
	string m_bucket;

	int download_file();
	string get_bucket();
	void create_directory();

};
