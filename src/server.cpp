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

#include <random>

#include <iostream>
#include <signal.h>
#include "fcgio.h"
#include "config.h"
#include "logger/logger.h"
#include "worker/worker.h"
#include "hash_table_helper/hash_table_helper.h"
#include "full_text/full_text.h"
#include "full_text/full_text_record.h"
#include "profiler/profiler.h"
#include "full_text/full_text.h"
#include "indexer/console.h"
#include "json.hpp"
#include "server/search_server.h"
#include "server/url_server.h"

#include <fstream>

using namespace std;

void runner(void) {
	const size_t data_len = 16000000ull*24ull;
	char *random_data = new char[data_len];

	const size_t file_len = 8000000ull*24ull*200ull*5ull*4ull;
	hash<string> hasher;
	while (true) {
		ifstream infile("/mnt/0/asd");
		size_t rnd = hasher(to_string(rand())) % (file_len - data_len);
		infile.seekg(rnd);
		cout << hasher(to_string(rand())) << endl;
		profiler::instance prof("reading data");
		infile.read((char *)random_data, data_len);
		prof.stop();
		prof.print();
	}
}

int main(int argc, const char **argv) {

	struct sigaction act{SIG_IGN};
	sigaction(SIGPIPE, &act, NULL);

	logger::start_logger_thread();

	if (getenv("ALEXANDRIA_CONFIG") != NULL) {
		config::read_config(getenv("ALEXANDRIA_CONFIG"));
	} else {
		config::read_config("/etc/alexandria.conf");
	}

	const string arg(argc > 1 ? argv[1] : "");

	if (argc == 2 && arg == "domain_info") {

		indexer::domain_info_server();

	}

	if (argc == 2 && arg == "url_server") {

		server::url_server();

	}

	if (argc == 2 && arg == "search_server") {

		server::search_server();

	}

	/*if (argc == 1 && full_text::is_indexed()) {

		//worker::start_urlstore_server();

		cout << "starting download server" << endl;
		worker::start_download_server();
		worker::start_server();

	} else if (argc == 1 && !full_text::is_indexed()) {

		worker::status status;
		status.items = full_text::total_urls_in_batches();
		status.items_indexed = 0;
		status.start_time = profiler::timestamp();
		worker::start_status_server(status);

		full_text::index_all_batches("main_index", "main_index", status);
		full_text::index_all_link_batches("link_index", "domain_link_index", "link_index", "domain_link_index", status);

		vector<hash_table::hash_table_shard_builder *> shards = hash_table_helper::create_shard_builders("main_index");
		hash_table_helper::optimize(shards);
		hash_table_helper::delete_shard_builders(shards);

	} else if (arg == "link") {

		full_text::index_all_link_batches("link_index", "domain_link_index", "link_index", "domain_link_index");

	} else if (arg == "count_link") {

		//full_text::count_all_links("main_index", status);

	} else if (arg == "optimize") {

		vector<hash_table::hash_table_shard_builder *> shards = hash_table_helper::create_shard_builders("main_index");
		hash_table_helper::optimize(shards);
		hash_table_helper::delete_shard_builders(shards);

	} else if (arg == "truncate_link") {

		hash_table_helper::truncate("link_index");
		hash_table_helper::truncate("domain_link_index");

		full_text::truncate_index("link_index");
		full_text::truncate_index("domain_link_index");

	} else if (arg == "truncate") {

		full_text::url_to_domain url_store("main_index");
		url_store.truncate();

		full_text::truncate_url_to_domain("main_index");
		full_text::truncate_index("main_index");
		full_text::truncate_index("link_index");
		full_text::truncate_index("domain_link_index");

		hash_table_helper::truncate("main_index");
		hash_table_helper::truncate("link_index");
		hash_table_helper::truncate("domain_link_index");
	}*/

	logger::join_logger_thread();

	return 0;
}

