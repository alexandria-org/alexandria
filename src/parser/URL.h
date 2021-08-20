
#pragma once

#include <iostream>
#include <functional>
#include <boost/algorithm/string/join.hpp>
#include "common/common.h"
#include "system/SubSystem.h"

using namespace std;

class URL {

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
	uint64_t domain_link_hash(const URL &target_url, const string &link_text) const;

	string host() const;
	string host_top_domain() const;
	string scheme() const;
	string path() const;
	string path_with_query() const;
	map<string, string> query() const;
	string host_reverse() const;
	string unescape(const string &str) const;
	string domain_without_tld() const;
	uint32_t size() const;

	float harmonic(const SubSystem *sub_system) const;

	friend istream &operator >>(istream &ss, URL &url);
	friend ostream &operator <<(ostream& os, const URL& url);

private:

	std::hash<std::string> m_hasher;
	string m_url_string;
	string m_host;
	string m_host_reverse;
	string m_scheme;
	string m_path;
	string m_query;
	int m_status;

	int parse();
	inline void remove_www(string &path);


};
