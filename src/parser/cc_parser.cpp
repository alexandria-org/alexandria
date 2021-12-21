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

// main.cpp
#include "config.h"
#include "parser/Warc.h"
#include "system/ThreadPool.h"
#include "system/Logger.h"
#include "text/Text.h"
#include "transfer/Transfer.h"
#include <iostream>

using namespace std;

namespace Parser {

	void run_downloader(const string &warc_path) {

		Warc::Parser pp;
		Warc::multipart_download("http://commoncrawl.s3.amazonaws.com/" + warc_path, [&pp](const string &chunk) {
			stringstream ss(chunk);
			pp.parse_stream(ss);
		});

		LOG_INFO("uploading: " + warc_path);
		int error;
		error = Transfer::upload_gz_file(Warc::get_result_path(warc_path), pp.result());
		error = Transfer::upload_gz_file(Warc::get_link_result_path(warc_path), pp.link_result());

		if (error) {
			LOG_INFO("error uploading: " + warc_path);
		}
	}

	void start_downloaders(const vector<string> &warc_paths) {
		const size_t num_threads = 48;
		ThreadPool pool(num_threads);
		vector<future<void>> results;

		for (const string &warc_path : warc_paths) {
			results.emplace_back(pool.enqueue([warc_path, num_threads] {
				sleep(rand() % (num_threads * 2));
				run_downloader(warc_path);
			}));
		}

		for(auto &&result: results) {
			result.get();
		}
	}

	vector<string> download_warc_paths() {
		int error;
		string content = Transfer::file_to_string("nodes/" + Config::node + "/warc.paths", error);
		if (error == Transfer::ERROR) return {};

		content = Text::trim(content);

		vector<string> raw_warc_paths;
		boost::algorithm::split(raw_warc_paths, content, boost::is_any_of("\n"));

		vector<string> warc_paths;
		for (const string &warc_path : raw_warc_paths) {
			if (Text::trim(warc_path).size()) {
				warc_paths.push_back(Text::trim(warc_path));
			}
		}

		return warc_paths;
	}

	bool upload_warc_paths(const vector<string> &warc_paths) {
		string content = boost::algorithm::join(warc_paths, "\n");
		int error = Transfer::upload_file("nodes/" + Config::node + "/warc.paths", content);
		return error == Transfer::OK;
	}

	void warc_downloader() {

		const size_t timeout = 300;
		const size_t limit = 500;

		// main loop
		while (true) {

			// Check if there are any urls to digest every 'timeout' minutes.
			vector<string> warc_paths = download_warc_paths();

			if (warc_paths.size() > 0) {
				// Digest 'limit' number of warc paths.
				vector<string> warc_paths_to_download;
				while (warc_paths_to_download.size() < limit && warc_paths.size() > 0) {
					warc_paths_to_download.push_back(warc_paths.back());
					warc_paths.pop_back();
				}

				if (upload_warc_paths(warc_paths)) {
					start_downloaders(warc_paths_to_download);
				} else {
					LOG_INFO("Fatal, could not upload warc paths, will not download");
				}
			}

			sleep(timeout);
		}
	}
}

