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

#include <curl/curl.h>
#include <iostream>
#include <sstream>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

namespace transfer {

	const std::string username = "alexandria";
	const std::string password = "wmXN6U4u";

	const int OK = 0;
	const int ERROR = 1;

	struct Response {
		std::string body;
		size_t code;
	};

	size_t curl_stringstream_writer(void *ptr, size_t size, size_t nmemb, std::stringstream *ss);
	size_t curl_ostream_writer(void *ptr, size_t size, size_t nmemb, std::ostream *os);

	void prepare_curl(CURL *curl);

	std::string file_to_string(const std::string &file_path, int &error);
	std::string gz_file_to_string(const std::string &file_path, int &error);

	void file_to_stream(const std::string &file_path, std::ostream &output_stream, int &error);
	void gz_file_to_stream(const std::string &file_path, std::ostream &output_stream, int &error);

	void url_to_string(const std::string &url, std::string &buffer, int &error);

	std::vector<std::string> download_gz_files_to_disk(const std::vector<std::string> &files_to_download);
	void delete_downloaded_files(const std::vector<std::string> &files);

	// Make a http HEAD request and return the content length. Return 0 on failure and sets the error parameter to transfer::ERROR
	size_t head_content_length(const std::string &url, int &error);

	int upload_file(const std::string &path, const std::string &data);
	int upload_gz_file(const std::string &path, const std::string &data);

	/*
	 * Perform simple GET request and return response.
	 * */
	Response get(const std::string &url);
	Response get(const std::string &url, const std::vector<std::string> &headers);

	/*
	 * Perform simple POST request and return response.
	 * */
	Response post(const std::string &url, const std::string &data);
	Response post(const std::string &url, const std::string &data, const std::vector<std::string> &headers);

	/*
	 * Perform simple PUT request and return response.
	 * */
	Response put(const std::string &url, const std::string &data);

}
