
#pragma once

#include <iostream>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include "TextBase.h"
#include "SearchResult.h"

using namespace boost::iostreams;
using namespace std;

class CCApi : public TextBase {

public:
	CCApi(const Aws::S3::S3Client &s3_client);
	~CCApi();

	string query(const string &query);

private:
	Aws::S3::S3Client m_s3_client;
	string m_query;
	vector<string> m_words;

	string api_failure(const string &reason);
	vector<SearchResult> get_results();
	vector<SearchResult> download_index();
	vector<SearchResult> handle_index_file(filtering_istream &stream);

};