
#pragma once

#include <iostream>
#include <sstream>

#include "parser/URL.h"
#include "abstract/TextBase.h"
#include "LinkSearchResult.h"

using namespace std;

class SearchResult : public TextBase {

public:

	SearchResult(const string &line);
	~SearchResult();

	string url() const;
	string url_clean() const;
	string host() const;
	string title() const;
	string snippet() const;
	double score() const;
	bool should_include() const;

	void calculate_score(const string &query, const vector<string> &words);
	void add_links(const vector<LinkSearchResult> &links);
	void add_domain_links(const vector<LinkSearchResult> &links);

private:
	
	// Score modifiers
	int m_inlink_count = 0;
	double m_inlink_score = 0.0;
	vector<LinkSearchResult> m_links;
	int m_centrality;
	size_t m_domains_linking_count;
	double m_score;
	bool m_should_include;

	// Data
	string m_title;
	string m_snippet;
	string m_url;
	string m_url_clean;
	string m_path;
	string m_host;

};
