
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
	response.set_debug("files_downloaded", m_files_downloaded);
	response.set_debug("file_size", m_file_size);
	response.set_debug("file_unzipped_size", m_file_unzipped_size);
	response.set_debug("download_time", m_download_time);
	response.set_debug("parse_time", m_parse_time);
	response.set_debug("sort_time", m_sort_time);
	response.set_debug("num_lines", m_num_lines);

	return response;
}

vector<SearchResult> CCApi::get_results() {
	download();

	Profiler parser_timer("Parse results");
	parse_results();

	m_parse_time = parser_timer.get();

	Profiler sorter_timer("Sorter timer");
	sort(m_results[m_words[0]].begin(), m_results[m_words[0]].end(), [&](const SearchResult& a, const SearchResult& b) {
		return (a.score() > b.score());
	});

	map<string, string> host_map;
	size_t idx = 0;
	vector<SearchResult> ret;
	const size_t limit = 10;
	for (const SearchResult &result : m_results[m_words[0]]) {
		if (host_map.find(result.host()) == host_map.end()) {
			host_map[result.host()] = result.host();
			ret.push_back(result);
			if (idx >= limit) break;
			idx++;
		}
	}

	m_sort_time = sorter_timer.get();

	return ret;
}

void CCApi::download() {

	ThreadPool pool(CC_API_THREADS_DOWNLOADING);
	std::map<string, std::future<string>> results;
	std::map<string, std::future<string>> links;

	for (const string &word : m_words) {

		string path = m_index_base + "/index_" + word + ".tsv.gz";
		results.emplace(make_pair(word, pool.enqueue([this, path] {
			return download_index(path);
		})));

		path = m_index_base + "/index_" + word + ".link.tsv.gz";
		links.emplace(make_pair(word, pool.enqueue([this, path] {
			return download_index(path);
		})));

		break;
	}

	Profiler profile("Index Download");

	// Wait for everything to download.
	for (auto &iter: results) {
		m_results_string[iter.first] = iter.second.get();
	}

	for (auto &iter: links) {
		m_links_string[iter.first] = iter.second.get();
	}

	m_download_time = profile.get();
}

void CCApi::parse_results() {
	Profiler profile("Parsing");

	for (const string &word : m_words) {
		parse_link_result(word);
	}
	for (const string &word : m_words) {
		parse_search_result(word);
	}
}

void CCApi::parse_search_result(const string &word) {
	stringstream ss(m_results_string[word]);
	string line;
	while (getline(ss, line)) {
		m_num_lines++;
		SearchResult search_result(line);
		search_result.add_links(m_links[word][search_result.url_clean()]);
		search_result.add_domain_links(m_domain_links[word][search_result.host()]);
		search_result.calculate_score(m_query, m_words);
		if (search_result.should_include()) {
			m_results[word].emplace_back(search_result);
		}
	}
}

void CCApi::parse_link_result(const string &word) {
	stringstream ss(m_links_string[word]);
	string line;
	set<string> deduplicate;
	while (getline(ss, line)) {
		m_num_lines++;
		LinkResult link(line);
		if (link.match(m_query) && link.centrality() >= 0) {
			string key = link.m_target_host + link.m_target_path + link.m_source_host;
			if (deduplicate.find(key) == deduplicate.end()) {
				deduplicate.insert(key);
				m_links[word][link.m_target_host + link.m_target_path].emplace_back(link);
				m_domain_links[word][link.m_target_host].emplace_back(link);
			}
		}
	}
}

/*


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
*/

/*vector<SearchResult> CCApi::handle_index_file(filtering_istream &stream) {
	/*
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

	return ret;;
}*/

string CCApi::download_index(const string &index_path) {
	Aws::S3::Model::GetObjectRequest request;
	request.SetBucket("alexandria-index");
	request.SetKey(index_path);

	cout << "Downloading: " << index_path << endl;

	auto outcome = m_s3_client.GetObject(request);

	if (outcome.IsSuccess()) {

		auto &stream = outcome.GetResultWithOwnership().GetBody();

		m_files_downloaded++;
		m_file_size += outcome.GetResultWithOwnership().GetContentLength();

		cout << "Downloaded: " << index_path << " " << (float)m_file_size/(1024*1024) << "mb" << endl;

		filtering_istream decompress_stream;
		decompress_stream.push(gzip_decompressor());
		decompress_stream.push(stream);

		const string ret(istreambuf_iterator<char>(decompress_stream), {});
		m_file_unzipped_size += ret.size();
		return ret;
	}

	return "";
}