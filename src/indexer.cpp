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

#include "full_text/full_text.h"
#include <iostream>
#include "config.h"
#include "logger/logger.h"
#include "tools/splitter.h"
#include "tools/counter.h"
#include "tools/download.h"
#include "tools/calculate_harmonic.h"
#include "tools/generate_url_lists.h"
#include "tools/find_links.h"
#include "URL.h"
#include "worker/worker.h"
#include "indexer/console.h"
#include <iostream>
#include <set>
#include "url_store/url_store.h"

using namespace std;

void help() {
	cout << "Usage: ./tools [OPTION]..." << endl;
	cout << "--split run splitter" << endl;
	cout << "--harmonic-hosts create file /tmp/hosts.txt with hosts for harmonic centrality" << endl;
	cout << "--harmonic-links create file /tmp/edges.txt for edges for harmonic centrality" << endl;
	cout << "--harmonic calculates harmonic centrality" << endl;
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

	if (arg == "--index") {
		full_text::index_all_batches("main_index", "main_index");
	} else if (arg == "--split") {
		tools::run_splitter();
	} else if (arg == "--count") {
		tools::run_counter();
		} else if (arg == "--count-domains") {
		tools::run_counter_per_domain(argv[2]);
	} else if (arg == "--count-links") {
		tools::count_all_links();
	} else if (arg == "--make-urls" && argc > 2) {
		tools::generate_url_lists(argv[2]);
	} else if (arg == "--urlstore") {
		worker::start_urlstore_server();
		worker::wait_for_urlstore_server();
	} else if (arg == "--split-with-links") {
		tools::run_splitter_with_links();
	} else if (arg == "--download-batch") {
		tools::download_batch(string(argv[2]));
	} else if (arg == "--prepare-batch") {
		tools::prepare_batch(stoull(string(argv[2])));
	} else if (arg == "--harmonic-hosts") {
		tools::calculate_harmonic_hosts();
	} else if (arg == "--harmonic-links") {
		tools::calculate_harmonic_links();
	} else if (arg == "--harmonic") {
		tools::calculate_harmonic();
	} else if (arg == "--host-hash") {
		URL url(argv[2]);
		cout << url.host_hash() << endl;
	} else if (arg == "--host-hash-mod") {
		URL url(argv[2]);
		cout << url.host_hash() % stoull(argv[3]) << endl;
	} else if (arg == "--find-links") {
		tools::find_links();
	} else if (arg == "--console") {
		indexer::console();
	} else if (arg == "--index-urls") {
		indexer::index_urls(argv[2]);
	} else if (arg == "--index-links") {
		indexer::index_links(argv[2]);
	} else if (arg == "--index-words") {
		indexer::index_words(argv[2]);
	} else if (arg == "--print-info") {
		indexer::print_info();
	} else if (arg == "--calc-scores") {
		indexer::calc_scores();
	} else if (arg == "--truncate-words") {
		indexer::truncate_words();
	} else if (arg == "--truncate-links") {
		indexer::truncate_links();
	} else {
		help();
	}

	logger::join_logger_thread();

	return 0;
}
