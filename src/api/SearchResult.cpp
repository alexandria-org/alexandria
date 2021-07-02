
#include "SearchResult.h"

SearchResult::SearchResult(const string &line) {
	stringstream ss(line);

	string col;

	size_t pos_start = 0;
	size_t pos_end = 0;
	size_t col_num = 0;
	while (pos_end != string::npos) {
		pos_end = line.find('\t', pos_start);
		const size_t len = pos_end - pos_start;
		if (col_num == 1) {
			m_url = line.substr(pos_start, len);
		}
		if (col_num == 2) {
			m_centrality = stoi(line.substr(pos_start, len));
		}
		if (col_num == 3) {
			m_title = line.substr(pos_start, len);
		}
		if (col_num == 4) {
			m_snippet = line.substr(pos_start, len);
		}

		pos_start = pos_end + 1;
		col_num++;
	}

	// Remove https?:// from url.
	size_t protocol_start = m_url.find("//", 0);
	m_url_clean = m_url.substr(protocol_start + 2);
	size_t www_start = m_url_clean.find("www.");
	if (www_start == 0) {
		m_url_clean = m_url_clean.substr(4);
	}

	size_t path_start = m_url_clean.find('/');
	m_path = m_url_clean.substr(path_start);
	m_host = m_url_clean.substr(0, path_start);
}

SearchResult::~SearchResult() {

}

string SearchResult::url() const {
	return m_url;
}

string SearchResult::url_clean() const {
	return m_url_clean;
}

string SearchResult::host() const {
	return m_host;
}

string SearchResult::title() const {
	return m_title;
}

string SearchResult::snippet() const {
	return m_snippet;
}

double SearchResult::score() const {
	return m_score;
}

bool SearchResult::should_include() const {
	return m_should_include;
}

void SearchResult::calculate_score(const string &query, const vector<string> &words) {

	double modifier = 1.0;

	// If we find the whole query in title or in snippet
	if (lower_case(m_title).find(query) != string::npos || lower_case(m_snippet).find(query) != string::npos) {
		modifier += 1.0;
	}

	// If the URL length is...
	if (m_host.length() > 40) {
		modifier *= 0.1;
	}
	if (m_path.length() > 140) {
		modifier *= 0.9;
	}
	if (m_path == "/") {
		modifier += 1.0;
	}

	if (m_snippet.size() == 0) {
		modifier -= 1.0;
	}

	// Only include search results where all the words are present.
	m_should_include = true;
	const string lower_title = lower_case(m_title);
	const string lower_snippet = lower_case(m_snippet);
	for (const string &word : words) {
		if (!(lower_title.find(word) == 0 || lower_title.find(" " + word) != string::npos) &&
			!(lower_snippet.find(word) == 0 || lower_snippet.find(" " + word) != string::npos)) {
			m_should_include = false;
		}
	}

	m_score = m_centrality * modifier * (1.0 + m_inlink_score / 10.0);
}

void SearchResult::add_links(const vector<LinkSearchResult> &links) {
	for (const LinkSearchResult &link : links) {
		m_inlink_score += 5*(double)link.m_centrality / 100000000.0;
	}
	m_inlink_count = links.size();
}

void SearchResult::add_domain_links(const vector<LinkSearchResult> &links) {
	for (const LinkSearchResult &link : links) {
		m_inlink_score += (double)link.m_centrality / 100000000.0;
	}
	m_inlink_count = links.size();
}
