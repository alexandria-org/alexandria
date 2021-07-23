
#include <iostream>
#include "fcgio.h"
#include "parser/URL.h"

#include "post_processor/PostProcessor.h"
#include "api/ApiResponse.h"

#include "hash_table/HashTable.h"

#include "full_text/FullTextIndex.h"
#include "full_text/FullTextRecord.h"

#include "link_index/LinkIndex.h"
#include "link_index/LinkFullTextRecord.h"

#include "system/Logger.h"

using namespace std;

void run_search_query(const string &query, FCGX_Request &request, HashTable &hash_table, FullTextIndex<FullTextRecord> &fti,
	FullTextIndex<LinkFullTextRecord> &link_fti) {

	Profiler profiler("total");

	size_t total;
	vector<FullTextRecord> results = fti.search_phrase(link_fti, query, 1000, total);

	PostProcessor post_processor(query);

	vector<ResultWithSnippet> with_snippets;
	for (FullTextRecord &res : results) {
		const string tsv_data = hash_table.find(res.m_value);
		with_snippets.emplace_back(ResultWithSnippet(tsv_data, res));
	}

	post_processor.run(with_snippets);

	ApiResponse response(with_snippets, total, profiler.get());

	streambuf *cin_streambuf  = cin.rdbuf();
	streambuf *cout_streambuf = cout.rdbuf();
	streambuf *cerr_streambuf = cerr.rdbuf();

	fcgi_streambuf cin_fcgi_streambuf(request.in);
	fcgi_streambuf cout_fcgi_streambuf(request.out);
	fcgi_streambuf cerr_fcgi_streambuf(request.err);

	cin.rdbuf(&cin_fcgi_streambuf);
	cout.rdbuf(&cout_fcgi_streambuf);
	cerr.rdbuf(&cerr_fcgi_streambuf);

	cout << "Content-type: application/json\r\n"
	     << "\r\n"
	     << response;

	cin.rdbuf(cin_streambuf);
	cout.rdbuf(cout_streambuf);
	cerr.rdbuf(cerr_streambuf);
}

void run_link_query(const string &query, FCGX_Request &request, HashTable &hash_table, FullTextIndex<LinkFullTextRecord> &fti) {

	Profiler profiler("total");

	size_t total;
	vector<LinkFullTextRecord> results = fti.search_phrase(query, 1000, total);

	PostProcessor post_processor(query);

	vector<ResultWithSnippet> with_snippets;
	for (LinkFullTextRecord &res : results) {
		const string tsv_data = hash_table.find(res.m_value);
		cout << res.m_value << ": " << tsv_data << endl;
		//with_snippets.emplace_back(ResultWithSnippet(tsv_data, res.m_score));
	}

	post_processor.run(with_snippets);

	ApiResponse response(with_snippets, total, profiler.get());

	streambuf *cin_streambuf  = cin.rdbuf();
	streambuf *cout_streambuf = cout.rdbuf();
	streambuf *cerr_streambuf = cerr.rdbuf();

	fcgi_streambuf cin_fcgi_streambuf(request.in);
	fcgi_streambuf cout_fcgi_streambuf(request.out);
	fcgi_streambuf cerr_fcgi_streambuf(request.err);

	cin.rdbuf(&cin_fcgi_streambuf);
	cout.rdbuf(&cout_fcgi_streambuf);
	cerr.rdbuf(&cerr_fcgi_streambuf);

	cout << "Content-type: application/json\r\n"
	     << "\r\n"
	     << response;

	cin.rdbuf(cin_streambuf);
	cout.rdbuf(cout_streambuf);
	cerr.rdbuf(cerr_streambuf);
}

void run_url_lookup(const string &url_str, FCGX_Request &request, HashTable &hash_table) {

	URL url(url_str);

	const string response = hash_table.find(url.hash());

	streambuf *cin_streambuf  = cin.rdbuf();
	streambuf *cout_streambuf = cout.rdbuf();
	streambuf *cerr_streambuf = cerr.rdbuf();

	fcgi_streambuf cin_fcgi_streambuf(request.in);
	fcgi_streambuf cout_fcgi_streambuf(request.out);
	fcgi_streambuf cerr_fcgi_streambuf(request.err);

	cin.rdbuf(&cin_fcgi_streambuf);
	cout.rdbuf(&cout_fcgi_streambuf);
	cerr.rdbuf(&cerr_fcgi_streambuf);

	cout << "Content-type: text/html\r\n"
	     << "\r\n"
	     << response;

	cin.rdbuf(cin_streambuf);
	cout.rdbuf(cout_streambuf);
	cerr.rdbuf(cerr_streambuf);
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
	FullTextIndex<FullTextRecord> fti("main_index");
	FullTextIndex<LinkFullTextRecord> link_fti("link_index");
	HashTable hash_table_link("link_index");

	LogInfo("Server has started...");

	while (FCGX_Accept_r(&request) == 0) {
		LogInfo("Serving request");

		string uri = FCGX_GetParam("REQUEST_URI", request.envp);

		URL url("http://alexandria.org" + uri);

		auto query = url.query();

		if (query.find("q") != query.end()) {
			run_search_query(query["q"], request, hash_table, fti, link_fti);
		} else if (query.find("l") != query.end()) {
			run_link_query(query["l"], request, hash_table_link, link_fti);
		} else if (query.find("u") != query.end()) {
			run_url_lookup(query["u"], request, hash_table);
		}
	}

	cin.rdbuf(cin_streambuf);
	cout.rdbuf(cout_streambuf);
	cerr.rdbuf(cerr_streambuf);

	return 0;
}

