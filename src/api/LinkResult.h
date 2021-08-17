
#pragma once

#include <iostream>
#include "parser/URL.h"
#include "link_index/LinkFullTextRecord.h"

using namespace std;

class LinkResult {

public:
	LinkResult(const string &tsv_data, const LinkFullTextRecord &res);
	~LinkResult();

	const URL &source_url() const { return m_source_url; };
	const URL &target_url() const { return m_target_url; };
	const string &link_text() const { return m_link_text; };
	const float &score() const { return m_score; };
	const uint64_t &link_hash() const { return m_link_hash; };

private:

	URL m_source_url;
	URL m_target_url;
	string m_link_text;
	float m_score;
	uint64_t m_link_hash;

};
