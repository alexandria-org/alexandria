
#pragma once

#include <iostream>
#include "parser/URL.h"
#include "full_text/FullTextRecord.h"

using namespace std;

class ResultWithSnippet {

public:
	ResultWithSnippet(const string &tsv_data, const FullTextRecord &res);
	~ResultWithSnippet();

	const URL &url() const { return m_url; };
	const string &title() const { return m_title; };
	const string &snippet() const { return m_snippet; };
	const float &score() const { return m_score; };
	const uint64_t &domain_hash() const { return m_domain_hash; };

private:

	URL m_url;
	string m_title;
	string m_snippet;
	float m_score;
	uint64_t m_domain_hash;

	string make_snippet(const string &text) const;

};
