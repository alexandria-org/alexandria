
#pragma once

#include <iostream>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include "ApiResponse.h"
#include "TextBase.h"
#include "SearchResult.h"
#include "LinkResult.h"
#include "ThreadPool.h"
#include "TsvRow.h"

using namespace boost::iostreams;
using namespace std;

#define CC_API_THREADS_DOWNLOADING 32

class CCApi : public TextBase {

public:
	CCApi(const Aws::S3::S3Client &s3_client);
	~CCApi();

	ApiResponse query(const string &query);

private:
	Aws::S3::S3Client m_s3_client;
	string m_query;
	map<string, string> m_results_string;
	map<string, string> m_links_string;
	map<string, vector<SearchResult>> m_results;
	map<string, map<string, vector<LinkResult>>> m_links;
	map<string, map<string, vector<LinkResult>>> m_domain_links;

	vector<string> m_words;
	const string m_index_base = "main";

	size_t m_file_size = 0;
	size_t m_file_unzipped_size = 0;

	size_t m_files_downloaded = 0;
	size_t m_download_time = 0;
	size_t m_parse_time = 0;
	size_t m_sort_time = 0;
	size_t m_num_lines = 0;

	vector<SearchResult> get_results();
	void download();
	void parse_results();
	void parse_search_result(const string &index_name);
	void parse_link_result(const string &index_name);
	//vector<SearchResult> handle_index_file(filtering_istream &stream);
	void sort_results();
	vector<SearchResult> get_top_results();

	string download_index(const string &index_path);

};
