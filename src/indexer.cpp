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
#include "config.h"
#include "logger/logger.h"
#include "tools/splitter.h"
#include "tools/counter.h"
#include "tools/download.h"
#include "tools/calculate_harmonic.h"
#include "tools/generate_url_lists.h"
#include "tools/find_links.h"
#include "URL.h"
#include "indexer/console.h"
#include <iostream>
#include <set>
#include "indexer/sharded_index.h"
#include "indexer/level.h"
#include "indexer/url_level.h"
#include "transfer/transfer.h"

using namespace std;

void help() {
	cout << "Usage: ./tools [OPTION]..." << endl;
	cout << "--split run splitter" << endl;
	cout << "--harmonic-hosts create file /tmp/hosts.txt with hosts for harmonic centrality" << endl;
	cout << "--harmonic-links create file /tmp/edges.txt for edges for harmonic centrality" << endl;
	cout << "--harmonic calculates harmonic centrality" << endl;
}

int main(int argc, const char **argv) {

	//logger::start_logger_thread();
	//logger::verbose(true);

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

	if (arg == "--split") {
		tools::run_splitter();
	} else if (arg == "--count") {
		tools::run_counter();
		} else if (arg == "--count-domains") {
		tools::run_counter_per_domain(argv[2]);
	} else if (arg == "--make-urls" && argc > 2) {
		tools::generate_url_lists(argv[2]);
	} else if (arg == "--split-with-links") {

		/*
		 * split with links takes all the URL batches and splits them into smaller NODE-{node id} folders
		 * with links means it only takes URLs with direct links in them. this is a major
		 * optimization and makes our target index much much smaller.
		 *
		 * */
		tools::run_split_urls_with_direct_links();
	} else if (arg == "--split-links") {

		/*
		 * split links should run after --split-with-links because it takes all the link batches and splits
		 * them into LINK-{node id} folders but it ONLY takes links with target domain that is present in the
		 * URL files stored in the NODE- folders.
		 *
		 * */
		tools::run_split_links_with_relevant_domains();
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
	} else if (arg == "--url-hash") {
		URL url(argv[2]);
		cout << url.hash() << endl;
	} else if (arg == "--host-hash-mod") {
		URL url(argv[2]);
		cout << url.host_hash() % stoull(argv[3]) << endl;
	} else if (arg == "--find-links") {
		tools::find_links();
	} else if (arg == "--console") {
		indexer::console();
	} else if (arg == "--index-links") {
		indexer::index_links(argv[2]);
	} else if (arg == "--index-urls") {
		indexer::index_urls();
	} else if (arg == "--make-domain-index") {
		indexer::make_domain_index();
	} else if (arg == "--make-domain-index-scores") {
		indexer::make_domain_index_scores();
	} else if (arg == "--truncate-links") {
		indexer::truncate_links();
	} else if (arg == "--make-url-bloom") {
		indexer::make_url_bloom_filter();
	} else {
		help();
	}

	logger::join_logger_thread();

	return 0;
}
