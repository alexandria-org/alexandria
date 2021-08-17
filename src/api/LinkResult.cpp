
#include "LinkResult.h"
#include "text/Text.h"

LinkResult::LinkResult(const string &tsv_data, const LinkFullTextRecord &res)
: m_score(res.m_score), m_link_hash(res.m_value) {
	size_t pos_start = 0;
	size_t pos_end = 0;
	size_t col_num = 0;

	string source_host;
	string source_path;
	string target_host;
	string target_path;

	http://http://214th.blogspot.com/2016/03/from-potemkin-pictures-battle-cruiser.html links to http://url1.com/my_test_url with link text: Star Trek Guinan

	pos_end = tsv_data.find(" links to ", pos_start);
	size_t len = pos_end - pos_start;
	m_source_url = URL(tsv_data.substr(pos_start, len));
	pos_start = pos_end + string(" links to ").size();

	pos_end = tsv_data.find(" with link text: ", pos_start);
	len = pos_end - pos_start;
	m_target_url = URL(tsv_data.substr(pos_start, len));
	pos_start = pos_end + string(" with link text: ").size();

	m_link_text = tsv_data.substr(pos_start);

}

LinkResult::~LinkResult() {

}

