
#include <iostream>
#include "fcgio.h"
#include "parser/URL.h"

#include "post_processor/PostProcessor.h"
#include "api/ApiResponse.h"

#include "hash_table/HashTable.h"

#include "full_text/FullTextIndex.h"
#include "full_text/FullTextRecord.h"
#include "full_text/SearchMetric.h"

#include "api/Api.h"

#include "link_index/LinkIndex.h"
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

	vector<FullTextIndex<FullTextRecord> *> index_array;
	for (size_t partition = 0; partition < 8; partition++) {
		index_array.push_back(new FullTextIndex<FullTextRecord>("main_index_" + to_string(partition)));
	}

	vector<FullTextIndex<LinkFullTextRecord> *> link_index_array;
	for (size_t partition = 0; partition < 8; partition++) {
		link_index_array.push_back(new FullTextIndex<LinkFullTextRecord>("link_index_" + to_string(partition)));
	}

	LogInfo("Server has started...");

	while (FCGX_Accept_r(&request) == 0) {

		string uri = FCGX_GetParam("REQUEST_URI", request.envp);

		LogInfo("Serving request: " + uri);

		URL url("http://alexandria.org" + uri);

		auto query = url.query();

		stringstream response_stream;

		if (query.find("q") != query.end()) {
			Api::search(query["q"], hash_table, index_array, link_index_array, response_stream);
		} else if (query.find("s") != query.end()) {
			Api::word_stats(query["s"], index_array, link_index_array, hash_table.size(), hash_table_link.size(), response_stream);
		} else if (query.find("u") != query.end()) {
			Api::url(query["u"], hash_table, response_stream);
		} else if (query.find("l") != query.end()) {
			Api::search_links(query["u"], hash_table_link, link_index_array, response_stream);
		}

		output_response(request, response_stream);
	}

	for (FullTextIndex<FullTextRecord> *fti : index_array) {
		delete fti;
	}

	for (FullTextIndex<LinkFullTextRecord> *fti : link_index_array) {
		delete fti;
	}

	cin.rdbuf(cin_streambuf);
	cout.rdbuf(cout_streambuf);
	cerr.rdbuf(cerr_streambuf);

	return 0;
}

