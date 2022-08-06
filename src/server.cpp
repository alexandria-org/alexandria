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
#include "profiler/profiler.h"
#include "indexer/console.h"
#include "json.hpp"
#include "server/search_server.h"
#include "server/url_server.h"

#include <fstream>

using namespace std;

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

	logger::join_logger_thread();

	return 0;
}

