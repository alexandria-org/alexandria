
#pragma once

#ifndef FILE_SERVER
	#define FILE_SERVER "http://node0003.alexandria.org"
#endif

#include <curl/curl.h>
#include <iostream>
#include <sstream>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

using namespace std;

namespace Transfer {

	const string file_server = FILE_SERVER;
	const string username = "alexandria";
	const string password = "wmXN6U4u";

	const int OK = 0;
	const int ERROR = 1;

	size_t curl_stringstream_writer(void *ptr, size_t size, size_t nmemb, stringstream *ss);
	size_t curl_ostream_writer(void *ptr, size_t size, size_t nmemb, ostream *os);

	void prepare_curl(CURL *curl);

	string file_to_string(const string &file_path, int &error);
	string gz_file_to_string(const string &file_path, int &error);

	void file_to_stream(const string &file_path, ostream &output_stream, int &error);
	void gz_file_to_stream(const string &file_path, ostream &output_stream, int &error);

}
