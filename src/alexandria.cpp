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

#include <iostream>
#include "logger/logger.h"
#include "downloader/warc_downloader.h"
#include "downloader/merge_downloader.h"
#include "URL.h"
#include "hash_table2/hash_table.h"

using namespace std;

void help() {
	cout << "Usage: ./alexandria [OPTION]..." << endl;
	cout << "--downloader [commoncrawl-batch] [limit] [offset]" << endl;
	cout << "--downloader-merge" << endl;
	cout << "--url [URL]" << endl;
}

int main(int argc, const char **argv) {

	logger::start_logger_thread();
	logger::verbose(true);

	if (getenv("ALEXANDRIA_CONFIG") != NULL) {
		config::read_config(getenv("ALEXANDRIA_CONFIG"));
	} else {
		config::read_config("/etc/alexandria.conf");
	}

	if (argc < 2) {
		help();
		return 0;
	}

	const string arg(argc > 1 ? argv[1] : "");

	if (arg == "--downloader" && argc > 4) {
		downloader::warc_downloader(argv[2], std::stoull(argv[3]), std::stoull(argv[4]));
	} else if (arg == "--downloader-merge") {
		downloader::merge_downloader();
	} else if (arg == "--url" && argc > 2) {
		URL url(argv[2]);
		hash_table2::hash_table ht("all_urls", 1019);

		size_t ver = 0;
		std::string data = ht.find(url.hash(), ver);
		std::cout << ver << std::endl;
		std::cout << data << std::endl;
	} else {
		help();
	}

	logger::join_logger_thread();

	return 0;
}
