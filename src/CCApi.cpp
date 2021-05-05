
#include "CCApi.h"
#include <aws/core/utils/json/JsonSerializer.h>

using namespace Aws::Utils::Json;

CCApi::CCApi(const Aws::S3::S3Client &s3_client) {
	m_s3_client = s3_client;
}

CCApi::~CCApi() {

}

string CCApi::query(const string &query) {

	m_query = query;
	m_words = get_words(query);

	vector<SearchResult> results = get_results();

	JsonValue json("{}");
	if (!json.WasParseSuccessful()) {
		return api_failure("Failed to parse input JSON");
	}

	Aws::Utils::Array<Aws::Utils::Json::JsonValue> result_array(results.size());

	size_t idx = 0;
	for (const SearchResult &result : results) {
		JsonValue json_result;
		JsonValue string;
		json_result.WithObject("url", string.AsString(result.url()));
		json_result.WithObject("title", string.AsString(result.title()));
		json_result.WithObject("snippet", string.AsString(result.snippet()));
		result_array[idx] = json_result;
		idx++;
	}

	JsonValue json_results;
	json_results.AsArray(result_array);
	json.WithObject("results", json_results);

	auto v = json.View();

	return json.View().WriteReadable();
}

string CCApi::api_failure(const string &reason) {
	JsonValue response, json_string;

	response.WithObject("status", json_string.AsString("error"));
	response.WithObject("reason", json_string.AsString(reason));
	return response.View().WriteReadable();
}

vector<SearchResult> CCApi::get_results() {
	vector<SearchResult> results = download_index();
	return results;
}

vector<SearchResult> CCApi::download_index() {

	vector<SearchResult> results;

	for (const string &word : m_words) { 

		Aws::S3::Model::GetObjectRequest request;
		request.SetBucket("alexandria-index");
		request.SetKey("CC-MAIN-2021-10/index_" + word + ".tsv.gz");

		auto outcome = m_s3_client.GetObject(request);

		if (outcome.IsSuccess()) {

			auto &stream = outcome.GetResultWithOwnership().GetBody();

			filtering_istream decompress_stream;
			decompress_stream.push(gzip_decompressor());
			decompress_stream.push(stream);

			results = handle_index_file(decompress_stream);

			break;
		}
	}

	return results;
}

vector<SearchResult> CCApi::handle_index_file(filtering_istream &stream) {
	auto timer_start = std::chrono::high_resolution_clock::now();
	vector<SearchResult> results;
	size_t limit = 10;
	string line;
	while (getline(stream, line)) {
		SearchResult result(line, m_words, m_query);
		if (result.should_include()) {
			results.push_back(SearchResult(line, m_words, m_query));
		}
	}

	sort(results.begin(), results.end(), [&](const SearchResult& a, const SearchResult& b) {
		return (a.score() > b.score());
	});

	vector<SearchResult> ret;

	map<string, string> host_map;
	size_t idx = 0;
	for (const SearchResult &result : results) {
		if (host_map.find(result.get_host()) == host_map.end()) {
			host_map[result.get_host()] = result.get_host();
			ret.push_back(result);
			if (idx >= limit) break;
			idx++;
		}
	}

	auto timer_elapsed = std::chrono::high_resolution_clock::now() - timer_start;
	auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(timer_elapsed).count();

	cout << "sorting took: " << microseconds / 1000 << " milliseconds" << endl;

	return ret;
}