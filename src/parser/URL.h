
#pragma once

#include <iostream>
#include <functional>
#include <boost/algorithm/string/join.hpp>
#include "common/common.h"
#include "system/SubSystem.h"

#include "abstract/TextBase.h"

using namespace std;

class URL : public TextBase {

public:
	URL();
	URL(const string &url);
	URL(const string &host, const string &path);
	~URL();

	static string host_reverse(const string &host);
	static string host_reverse_top_domain(const string &host);

	void set_url_string(const string &url);
	string str() const;
	uint64_t hash() const;
	uint64_t host_hash() const;
	uint64_t link_hash(const URL &target_url, const string &link_text) const;
	string host() const;
	string path() const;
	map<string, string> query() const;
	float harmonic(const SubSystem *sub_system) const;
	string host_reverse() const;
	string unescape(const string &str) const;
	uint32_t size() const;

	friend istream &operator >>(istream &ss, URL &url);
	friend ostream &operator <<(ostream& os, const URL& url);

private:

	std::hash<std::string> m_hasher;
	string m_url_string;
	string m_host;
	string m_host_reverse;
	string m_path;
	string m_query;
	int m_status;

	int parse();
	inline void remove_www(string &path);


};
