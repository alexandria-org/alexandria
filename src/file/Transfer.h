/*
 * MIT License
 *
 * Alexandria.org
 *
 * Copyright (c) 2021 Josef Cullhed, <info@alexandria.org>, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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

	vector<string> download_gz_files_to_disk(const vector<string> files_to_download);
	void delete_downloaded_files(const vector<string> &files);

}
