
#pragma once

#include <iostream>
#include "parser/URL.h"

using namespace std;

class ResultWithSnippet {

public:
	ResultWithSnippet(const string &tsv_data, float score);
	~ResultWithSnippet();

	const URL &url() const { return m_url; };
	const string &title() const { return m_title; };
	const string &snippet() const { return m_snippet; };
	const float &score() const { return m_score; };

private:

	URL m_url;
	string m_title;
	string m_snippet;
	float m_score;

	string make_snippet(const string &text) const;

};
