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
#include "fcgio.h"
#include "config.h"
#include "system/Logger.h"
#include "api/Worker.h"
#include "hash_table/HashTableHelper.h"
#include "full_text/FullText.h"
#include "full_text/FullTextRecord.h"
#include "system/Profiler.h"
#include "full_text/FullText.h"

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
		Profiler::instance prof("reading data");
		infile.read((char *)random_data, data_len);
		prof.stop();
		prof.print();
	}
}

int main(int argc, const char **argv) {

	/*{
		vector<thread> threads;
		for (size_t i = 0; i < 10; i++) {
			threads.emplace_back(std::move(thread(runner)));
		}

		for (thread &th : threads) {
			th.join();
		}

	}
	return 0;
	{
		const size_t data_len = 8000000*24;
		char *random_data = new char[data_len];
		for (size_t i = 0; i < data_len; i++) random_data[i] = rand();

		ofstream outfile("/mnt/0/asd", ios::trunc);
		for (size_t i = 0; i < 200*5*4; i++) {
			outfile.write(random_data, data_len);
		}
	}

	return 0;*/

	Logger::start_logger_thread();

	if (getenv("ALEXANDRIA_CONFIG") != NULL) {
		Config::read_config(getenv("ALEXANDRIA_CONFIG"));
	} else {
		Config::read_config("/etc/alexandria.conf");
	}

	FullText::testing();
	return 0;

	/*Worker::test_search(string(argv[1]));

	Logger::join_logger_thread();

	return 0;*/

	const string arg(argc > 1 ? argv[1] : "");

	if (argc == 1 && FullText::is_indexed()) {

		cout << "starting download server" << endl;
		Worker::start_download_server();
		Worker::start_server();

	} else if (argc == 1 && !FullText::is_indexed()) {

		Worker::Status status;
		status.items = FullText::total_urls_in_batches();
		status.items_indexed = 0;
		status.start_time = Profiler::timestamp();
		Worker::start_status_server(status);

		FullText::index_all_batches("main_index", "main_index", status);
		FullText::index_all_link_batches("link_index", "domain_link_index", "link_index", "domain_link_index", status);

		vector<HashTableShardBuilder *> shards = HashTableHelper::create_shard_builders("main_index");
		HashTableHelper::optimize(shards);
		HashTableHelper::delete_shard_builders(shards);

	} else if (arg == "link") {

		FullText::index_all_link_batches("link_index", "domain_link_index", "link_index", "domain_link_index");

	} else if (arg == "count_link") {

		Worker::Status status;
		status.items = FullText::total_urls_in_batches();
		status.items_indexed = 0;
		status.start_time = Profiler::timestamp();
		Worker::start_status_server(status);

		FullText::count_all_links("main_index", status);

	} else if (arg == "optimize") {

		vector<HashTableShardBuilder *> shards = HashTableHelper::create_shard_builders("main_index");
		HashTableHelper::optimize(shards);
		HashTableHelper::delete_shard_builders(shards);

	} else if (arg == "truncate_link") {

		HashTableHelper::truncate("link_index");
		HashTableHelper::truncate("domain_link_index");

		FullText::truncate_index("link_index");
		FullText::truncate_index("domain_link_index");

	} else if (arg == "truncate") {

		UrlToDomain url_to_domain("main_index");
		url_to_domain.truncate();

		FullText::truncate_url_to_domain("main_index");
		FullText::truncate_index("main_index");
		FullText::truncate_index("link_index");
		FullText::truncate_index("domain_link_index");

		HashTableHelper::truncate("main_index");
		HashTableHelper::truncate("link_index");
		HashTableHelper::truncate("domain_link_index");
	}

	Logger::join_logger_thread();

	return 0;
}

