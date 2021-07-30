
#pragma once

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>

#include <boost/filesystem.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include "Transfer.h"
#include "common/common.h"
#include "TsvFile.h"

using namespace std;
using namespace boost::iostreams;

class TsvFileRemote : public TsvFile {

public:

	TsvFileRemote(const string &file_name);
	~TsvFileRemote();

	string get_path() const;

private:

	int download_file();
	void create_directory();

};
