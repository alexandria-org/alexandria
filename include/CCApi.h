
#pragma once

#include <iostream>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include "ApiResponse.h"
#include "TextBase.h"
#include "SearchResult.h"

using namespace boost::iostreams;
using namespace std;

class CCApi : public TextBase {

public:
	CCApi(const Aws::S3::S3Client &s3_client);
	~CCApi();

	ApiResponse query(const string &query);

private:
	Aws::S3::S3Client m_s3_client;
	string m_query;
	vector<string> m_words;

	size_t m_file_size;
	size_t m_file_unzipped_size;

	size_t m_download_time;
	size_t m_parse_time;
	size_t m_sort_time;
	size_t m_num_lines;

	vector<SearchResult> get_results();
	vector<SearchResult> download_index();
	vector<SearchResult> handle_index_file(filtering_istream &stream);

};