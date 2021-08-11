
#pragma once

#include <iomanip>
#include <aws/core/Aws.h>
#include <aws/lambda/LambdaClient.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/s3/S3Client.h>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/algorithm/string.hpp>

#include "file/TsvFileS3.h"
#include "file/TsvFileRemote.h"
#include "common/Dictionary.h"

using namespace boost::iostreams;
using namespace std;

class SubSystem {

public:

	SubSystem();
	~SubSystem();

	const Dictionary *domain_index() const;
	const Dictionary *dictionary() const;
	const Dictionary *full_text_dictionary() const;
	const vector<string> words() const;
	const Aws::S3::S3Client s3_client() const;
	const Aws::Lambda::LambdaClient lambda_client() const;

	string download_to_string(const string &bucket, const string &key) const;
	bool download_to_stream(const string &bucket, const string &key, ofstream &output_stream) const;

	void upload_from_string(const string &bucket, const string &key, const string &data) const;
	void upload_from_stream(const string &bucket, const string &key, ifstream &file_stream) const;
	void upload_from_stream(const string &bucket, const string &key, filtering_istream &compress_stream) const;
	void upload_from_stream(const string &bucket, const string &key, filtering_istream &compress_stream,
		size_t retries) const;

private:
	Dictionary *m_domain_index;
	Dictionary *m_dictionary;
	Dictionary *m_full_text_dictionary;
	Aws::S3::S3Client *m_s3_client;
	Aws::Lambda::LambdaClient *m_lambda_client;
	vector<string> m_words;

	void init_aws_api();
	void deinit_aws_api();
	Aws::Client::ClientConfiguration get_s3_config();

};
