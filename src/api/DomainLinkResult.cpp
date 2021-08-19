
#include "DomainLinkResult.h"
#include "text/Text.h"

DomainLinkResult::DomainLinkResult(const string &tsv_data, const DomainLinkFullTextRecord &res)
: m_score(res.m_score), m_link_hash(res.m_value) {

	string source_host;
	string source_path;
	string target_host;
	string target_path;

	size_t pos_start = 0;
	size_t pos_end = 0;
	size_t col_num = 0;
	while (pos_end != string::npos) {
		pos_end = tsv_data.find('\t', pos_start);
		const size_t len = pos_end - pos_start;
		if (col_num == 0) {
			source_host = tsv_data.substr(pos_start, len);
		}
		if (col_num == 1) {
			source_path = tsv_data.substr(pos_start, len);
		}
		if (col_num == 2) {
			target_host = tsv_data.substr(pos_start, len);
		}
		if (col_num == 3) {
			target_path = tsv_data.substr(pos_start, len);
		}
		if (col_num == 4) {
			m_link_text = tsv_data.substr(pos_start, len);
		}

		pos_start = pos_end + 1;
		col_num++;
	}

	m_source_url = URL(source_host, source_path);
	m_target_url = URL(target_host, target_path);

}

DomainLinkResult::~DomainLinkResult() {

}

