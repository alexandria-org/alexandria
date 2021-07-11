
#include <iostream>
#include "fcgio.h"
#include "parser/URL.h"

#include "post_processor/PostProcessor.h"
#include "api/ApiResponse.h"

#include "hash_table/HashTable.h"

#include "full_text/FullTextIndex.h"
#include "full_text/FullTextResult.h"

#include "link_index/LinkIndex.h"
#include "link_index/LinkResult.h"

#include "system/Logger.h"

using namespace std;

int main(void) {
	streambuf *cin_streambuf = cin.rdbuf();
	streambuf *cout_streambuf = cout.rdbuf();
	streambuf *cerr_streambuf = cerr.rdbuf();

	FCGX_Request request;

	FCGX_Init();

	FCGX_InitRequest(&request, 0, 0);

	HashTable hash_table("main_index");
	FullTextIndex fti("main_index");

	LogInfo("Server has started...");

	while (FCGX_Accept_r(&request) == 0) {
		fcgi_streambuf cin_fcgi_streambuf(request.in);
		fcgi_streambuf cout_fcgi_streambuf(request.out);
		fcgi_streambuf cerr_fcgi_streambuf(request.err);

		cin.rdbuf(&cin_fcgi_streambuf);
		cout.rdbuf(&cout_fcgi_streambuf);
		cerr.rdbuf(&cerr_fcgi_streambuf);

		string uri = FCGX_GetParam("REQUEST_URI", request.envp);

		URL url("http://alexandria.org" + uri);

		auto query = url.query();

		size_t total;
		vector<FullTextResult> results = fti.search_phrase(query["q"], 1000, total);

		PostProcessor post_processor(query["q"]);

		vector<ResultWithSnippet> with_snippets;
		for (FullTextResult &res : results) {
			const string tsv_data = hash_table.find(res.m_value);
			with_snippets.emplace_back(ResultWithSnippet(tsv_data));
		}

		post_processor.run(with_snippets);

		ApiResponse response(with_snippets, total);

		cout << "Content-type: application/json\r\n"
		     << "\r\n"
		     << response;
	}

	cin.rdbuf(cin_streambuf);
	cout.rdbuf(cout_streambuf);
	cerr.rdbuf(cerr_streambuf);

	return 0;
}

