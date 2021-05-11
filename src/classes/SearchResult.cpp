
#include "SearchResult.h"

SearchResult::SearchResult(const string line, const vector<string> &words, const string &query) {
	stringstream ss(line);

	string col;

	getline(ss, col, '\t');
	getline(ss, m_url, '\t');
	m_url_obj.set_url_string(m_url);
	getline(ss, col, '\t');
	m_score = stod(col);
	getline(ss, m_title, '\t');
	getline(ss, m_snippet, '\t');

	m_score *= score_modifier(words, query);
}

SearchResult::~SearchResult() {

}

string SearchResult::url() const {
	return m_url;
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

string SearchResult::get_host() const {
	return m_url_obj.host();
}

bool SearchResult::should_include() const {
	return m_should_include;
}

double SearchResult::score_modifier(const vector<string> &words, const string &query) {
	//if (m_url_obj.path())
	double modifier = 1.0;
	if (m_url_obj.path() == "/") {
		modifier += 1.0;
	}
	if (lower_case(m_title).find(query) != string::npos || lower_case(m_snippet).find(query) != string::npos) {
		modifier += 1.0;
	}
	if (m_url.length() > 100) {
		modifier -= 2.0;
	} else {
		modifier -= (double)m_url.length() * 0.02;
	}

	m_should_include = true;
	for (const string &word : words) {
		if (lower_case(m_title).find(word) == string::npos && lower_case(m_snippet).find(word) == string::npos &&
			lower_case(m_url).find(word) == string::npos) {
			m_should_include = false;
		}
	}
	return modifier;
}
