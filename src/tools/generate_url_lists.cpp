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

#include <vector>
#include <iostream>
#include <fstream>
#include "generate_url_lists.h"

#include <boost/filesystem.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost::filesystem;

namespace tools {

	vector<string> read_urls_with_many_links(const std::string &file_path) {

		std::ifstream infile(file_path);
		if (!infile.is_open()) return {};

		vector<string> ret_urls;

		boost::iostreams::filtering_istream decompress_stream;
		decompress_stream.push(boost::iostreams::gzip_decompressor());
		decompress_stream.push(infile);

		string line;
		while (getline(decompress_stream, line)) {
			vector<string> cols;
			boost::algorithm::split(cols, line, boost::is_any_of("\t"));
			if (stoull(cols[1]) > 1) {
				ret_urls.push_back(cols[0]);
			}
		}

		return ret_urls;
	}

	vector<string> read_urls(const std::string &path) {
		// Only read the first 10 files.
		vector<string> urls;
		for (size_t i = 1; i <= 10; i++) {
			string file_path = path + "/top_" + to_string(i) + ".gz";
			if (is_regular_file(file_path)) {
				vector<string> new_urls = read_urls_with_many_links(file_path);
				if (new_urls.size() == 0) break;
				urls.insert(urls.end(), new_urls.begin(), new_urls.end());
			}
		}

		return urls;
	}

	void generate_url_lists(const std::string &batch_path) {
		path pth(batch_path);
		directory_iterator end_iter;

		vector<string> urls;

		for (directory_iterator iter(pth); iter != end_iter; iter++) {
			if (is_directory(iter->path())) {
				string current_file = iter->path().string();
				vector<string> new_urls = read_urls(current_file);
				urls.insert(urls.end(), new_urls.begin(), new_urls.end());
			}
		}

		for (const string &url : urls) {
			cout << url << endl;
		}

	}

}
