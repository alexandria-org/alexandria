
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

	JsonValue message("{}");
	if (!message.WasParseSuccessful()) {
		return api_failure("Failed to parse input JSON");
	}

	Aws::Utils::Array<Aws::Utils::Json::JsonValue> result_array(results.size());

	size_t idx = 0;
	for (const SearchResult &result : results) {
		JsonValue json_result;
		JsonValue string;
		json_result.WithObject("url", string.AsString(result.url()));
		json_result.WithObject("title", string.AsString(clean_string(result.title())));
		json_result.WithObject("snippet", string.AsString(clean_string(result.snippet())));
		result_array[idx] = json_result;
		idx++;
	}

	JsonValue json_results, json_number;
	json_results.AsArray(result_array);
	message.WithObject("results", json_results);

	// Write some profiling data.
	message.WithObject("file_size", json_number.AsInteger(m_file_size));
	message.WithObject("file_unzipped_size", json_number.AsInteger(m_file_unzipped_size));
	message.WithObject("download_time", json_number.AsInteger(m_download_time));
	message.WithObject("parse_time", json_number.AsInteger(m_parse_time));
	message.WithObject("sort_time", json_number.AsInteger(m_sort_time));
	message.WithObject("num_lines", json_number.AsInteger(m_num_lines));

	JsonValue response, json_string;

	response.WithObject("status", json_string.AsString("success"));
	response.WithObject("message", message);
	return response.View().WriteCompact();
}

string CCApi::api_failure(const string &reason) {
	JsonValue response, json_string;

	response.WithObject("status", json_string.AsString("error"));
	response.WithObject("reason", json_string.AsString(reason));
	return response.View().WriteCompact();
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

		auto timer_start = chrono::high_resolution_clock::now();
		auto outcome = m_s3_client.GetObject(request);

		if (outcome.IsSuccess()) {

			auto &stream = outcome.GetResultWithOwnership().GetBody();

			m_file_size = outcome.GetResultWithOwnership().GetContentLength();

			auto timer_elapsed = std::chrono::high_resolution_clock::now() - timer_start;
			auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(timer_elapsed).count();

			m_download_time = microseconds / 1000;

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
	m_file_unzipped_size = 0;
	m_num_lines = 0;
	while (getline(stream, line)) {
		SearchResult result(line, m_words, m_query);
		m_file_unzipped_size += line.size();
		if (result.should_include()) {
			results.push_back(SearchResult(line, m_words, m_query));
		}
		m_num_lines++;
	}

	auto timer_elapsed = std::chrono::high_resolution_clock::now() - timer_start;
	auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(timer_elapsed).count();

	m_parse_time = microseconds / 1000;

	timer_start = std::chrono::high_resolution_clock::now();

	sort(results.begin(), results.end(), [&](const SearchResult& a, const SearchResult& b) {
		return (a.score() > b.score());
	});

	timer_elapsed = std::chrono::high_resolution_clock::now() - timer_start;
	microseconds = std::chrono::duration_cast<std::chrono::microseconds>(timer_elapsed).count();

	m_sort_time = microseconds / 1000;

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

	cout << "sorting took: " << microseconds / 1000 << " milliseconds" << endl;

	return ret;
}