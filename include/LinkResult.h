
#pragma once

#include <iostream>
#include <sstream>

#include "URL.h"
#include "TextBase.h"

using namespace std;

class LinkResult : public TextBase {

public:

	LinkResult(const string &line);
	~LinkResult();

	string url() const;
	string link_text() const;
	string snippet() const;
	double score() const;
	string get_host() const;
	bool should_include() const;
	int centrality() const;
	bool match(const string &query) const;

public:
	bool m_should_include;
	double m_score;
	int m_centrality;
	string m_url;
	URL m_url_obj;
	string m_title;
	string m_snippet;
	string m_link_text;
	string m_target_host;
	string m_target_path;
	string m_source_host;

};