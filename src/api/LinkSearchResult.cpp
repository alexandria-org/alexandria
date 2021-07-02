
#include "LinkSearchResult.h"

LinkSearchResult::LinkSearchResult(const string &line) {
	stringstream ss(line);

	string col;

	size_t pos_start = 0;
	size_t pos_end = 0;
	size_t col_num = 0;
	while (pos_end != string::npos) {
		pos_end = line.find('\t', pos_start);
		const size_t len = pos_end - pos_start;
		if (col_num == 1) {
			m_target_host = line.substr(pos_start, len);
		}
		if (col_num == 2) {
			m_target_path = line.substr(pos_start, len);
		}
		if (col_num == 3) {
			m_source_host = line.substr(pos_start, len);
		}

		if (col_num == 5) {
			m_centrality = stoi(line.substr(pos_start, len));
		}
		if (col_num == 6) {
			m_link_text = line.substr(pos_start, len);
		}

		pos_start = pos_end + 1;
		col_num++;
	}

	//m_score *= score_modifier(words, query);
}

LinkSearchResult::~LinkSearchResult() {

}

string LinkSearchResult::url() const {
	return m_url;
}

string LinkSearchResult::link_text() const {
	return m_link_text;
}

string LinkSearchResult::snippet() const {
	return m_snippet;
}

double LinkSearchResult::score() const {
	return m_score;
}

string LinkSearchResult::get_host() const {
	return m_url_obj.host();
}

bool LinkSearchResult::should_include() const {
	return m_should_include;
}

int LinkSearchResult::centrality() const {
	return m_centrality;
}

bool LinkSearchResult::match(const string &query) const {
	return lower_case(m_link_text).find(query) != string::npos;
}
