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
#include "parser/URL.h"

#include "post_processor/PostProcessor.h"
#include "api/ApiResponse.h"

#include "hash_table/HashTable.h"

#include "full_text/FullText.h"
#include "full_text/FullTextIndex.h"
#include "full_text/FullTextRecord.h"
#include "full_text/SearchMetric.h"

#include "api/Api.h"

#include "link_index/LinkFullTextRecord.h"

#include "system/Logger.h"

using namespace std;

void output_response(FCGX_Request &request, stringstream &response) {

	fcgi_streambuf cin_fcgi_streambuf(request.in);
	fcgi_streambuf cout_fcgi_streambuf(request.out);
	fcgi_streambuf cerr_fcgi_streambuf(request.err);

	istream is{&cin_fcgi_streambuf};
	ostream os{&cout_fcgi_streambuf};
	ostream errs{&cerr_fcgi_streambuf};

	os << "Content-type: application/json\r\n"
	     << "\r\n"
	     << response.str();

}

int main(void) {

	streambuf *cin_streambuf = cin.rdbuf();
	streambuf *cout_streambuf = cout.rdbuf();
	streambuf *cerr_streambuf = cerr.rdbuf();

	FCGX_Request request;

	FCGX_Init();

	int socket_id = FCGX_OpenSocket("127.0.0.1:8000", 20);
	if (socket_id < 0) {
		LogInfo("Could not open socket, exiting");
		return 1;
	}
	FCGX_InitRequest(&request, socket_id, 0);

	HashTable hash_table("main_index");
	HashTable hash_table_link("link_index");
	HashTable hash_table_domain_link("domain_link_index");

	vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("main_index", 8);
	vector<FullTextIndex<LinkFullTextRecord> *> link_index_array = FullText::create_index_array<LinkFullTextRecord>("link_index", 8);
	vector<FullTextIndex<DomainLinkFullTextRecord> *> domain_link_index_array =
		FullText::create_index_array<DomainLinkFullTextRecord>("domain_link_index", 8);

	LogInfo("Server has started...");

	while (FCGX_Accept_r(&request) == 0) {

		string uri = FCGX_GetParam("REQUEST_URI", request.envp);

		LogInfo("Serving request: " + uri);

		URL url("http://alexandria.org" + uri);

		auto query = url.query();

		stringstream response_stream;

		if (query.find("q") != query.end()) {
			Api::search(query["q"], hash_table, index_array, link_index_array, domain_link_index_array, response_stream);
		} else if (query.find("s") != query.end()) {
			Api::word_stats(query["s"], index_array, link_index_array, hash_table.size(), hash_table_link.size(), response_stream);
		} else if (query.find("u") != query.end()) {
			Api::url(query["u"], hash_table, response_stream);
		} else if (query.find("l") != query.end()) {
			Api::search_links(query["l"], hash_table_link, link_index_array, response_stream);
		} else if (query.find("d") != query.end()) {
			Api::search_domain_links(query["d"], hash_table_domain_link, domain_link_index_array, response_stream);
		}

		output_response(request, response_stream);
	}

	FullText::delete_index_array<FullTextRecord>(index_array);
	FullText::delete_index_array<LinkFullTextRecord>(link_index_array);
	FullText::delete_index_array<DomainLinkFullTextRecord>(domain_link_index_array);

	cin.rdbuf(cin_streambuf);
	cout.rdbuf(cout_streambuf);
	cerr.rdbuf(cerr_streambuf);

	return 0;
}

