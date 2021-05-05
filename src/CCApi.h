
#pragma once

#include <iostream>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>

#include "TextBase.h"

using namespace std;

class CCApi : public TextBase {

public:
	CCApi(const Aws::S3::S3Client &s3_client);
	~CCApi();

	string query(const string &query);

private:
	Aws::S3::S3Client m_s3_client;

};