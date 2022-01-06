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

#include "config.h"
#include "tools/Splitter.h"
#include "tools/Counter.h"
#include "tools/Download.h"
#include "tools/CalculateHarmonic.h"
#include <iostream>
#include <set>

using namespace std;

void help() {
	cout << "Usage: ./tools [OPTION]..." << endl;
	cout << "--split run splitter" << endl;
	cout << "--harmonic-hosts create file /tmp/hosts.txt with hosts for harmonic centrality" << endl;
	cout << "--harmonic-links create file /tmp/edges.txt for edges for harmonic centrality" << endl;
	cout << "--harmonic calculates harmonic centrality" << endl;
}

int main(int argc, const char **argv) {

	if (getenv("ALEXANDRIA_CONFIG") != NULL) {
		Config::read_config(getenv("ALEXANDRIA_CONFIG"));
	} else {
		Config::read_config("/etc/alexandria.conf");
	}

	if (argc < 2) {
		help();
		return 0;
	}

	const string arg(argv[1]);

	if (arg == "--split") {
		Tools::run_splitter();
	} else if (arg == "--count") {
		Tools::run_counter();
	} else if (arg == "--split-with-links") {
		Tools::run_splitter_with_links();
	} else if (arg == "--download-batch") {
		Tools::download_batch(string(argv[2]));
	} else if (arg == "--harmonic-hosts") {
		Tools::calculate_harmonic_hosts();
	} else if (arg == "--harmonic-links") {
		Tools::calculate_harmonic_links();
	} else if (arg == "--harmonic") {
		Tools::calculate_harmonic();
	} else {
		help();
	}

	return 0;
}
