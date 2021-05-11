
#pragma once

#include <iostream>
#include <sstream>

#include "URL.h"
#include "TextBase.h"

using namespace std;

class SearchResult : public TextBase {

public:

	SearchResult(const string line, const vector<string> &words, const string &query);
	~SearchResult();

	string url() const;
	string title() const;
	string snippet() const;
	double score() const;
	string get_host() const;
	bool should_include() const;

private:
	bool m_should_include;
	double m_score;
	string m_url;
	URL m_url_obj;
	string m_title;
	string m_snippet;

	double score_modifier(const vector<string> &words, const string &query);

};