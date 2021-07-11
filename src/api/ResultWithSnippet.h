
#pragma once

#include <iostream>
#include "parser/URL.h"

using namespace std;

class ResultWithSnippet {

public:
	ResultWithSnippet(const string &tsv_data);
	~ResultWithSnippet();

	URL url() const;
	string title() const;
	string snippet() const;

private:

	URL m_url;
	string m_title;
	string m_snippet;
	uint32_t m_score;

	string make_snippet(const string &text) const;

};
