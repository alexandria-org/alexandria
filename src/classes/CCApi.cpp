
#include "CCApi.h"

CCApi::CCApi(const Aws::S3::S3Client &s3_client) {
	m_s3_client = s3_client;
}

CCApi::~CCApi() {

}

ApiResponse CCApi::query(const string &query) {

	ApiResponse response;

	m_query = query;
	m_words = get_words(query);

	vector<SearchResult> results = get_results();

	response.set_results(results);
	response.set_debug("file_size", m_file_size);
	response.set_debug("file_unzipped_size", m_file_unzipped_size);
	response.set_debug("download_time", m_download_time);
	response.set_debug("parse_time", m_parse_time);
	response.set_debug("sort_time", m_sort_time);
	response.set_debug("num_lines", m_num_lines);

	return response;
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