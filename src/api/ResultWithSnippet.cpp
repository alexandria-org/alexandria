
#include "ResultWithSnippet.h"

ResultWithSnippet::ResultWithSnippet(const string &tsv_data, const FullTextRecord &res)
: m_score(res.m_score), m_domain_hash(res.m_domain_hash) {
	size_t pos_start = 0;
	size_t pos_end = 0;
	size_t col_num = 0;
	while (pos_end != string::npos) {
		pos_end = tsv_data.find('\t', pos_start);
		const size_t len = pos_end - pos_start;
		if (col_num == 0) {
			m_url = URL(tsv_data.substr(pos_start, len));
		}
		if (col_num == 1) {
			m_title = tsv_data.substr(pos_start, len);
		}
		if (col_num == 4) {
			m_snippet = make_snippet(tsv_data.substr(pos_start, len));
		}

		pos_start = pos_end + 1;
		col_num++;
	}
}

ResultWithSnippet::~ResultWithSnippet() {

}

string ResultWithSnippet::make_snippet(const string &text) const {
	string response = text.substr(0, 140);
	if (response.size() >= 140) response += "...";
	return response;
}

