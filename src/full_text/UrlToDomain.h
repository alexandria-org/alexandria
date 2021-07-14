
#pragma once

#include <iostream>
#include <fstream>
#include <unordered_map>

using namespace std;

class UrlToDomain {

public:
	UrlToDomain(const string &db_name);
	~UrlToDomain();

	void add_url(uint64_t url_hash, uint64_t domain_hash);
	void read();
	void write(size_t indexer_id);

	const unordered_map<uint64_t, uint64_t> &url_to_domain() const { return m_url_to_domain; };
	const unordered_map<uint64_t, size_t> &domains() const { return m_domains; };

private:
	const string m_db_name;
	unordered_map<uint64_t, uint64_t> m_url_to_domain;
	unordered_map<uint64_t, size_t> m_domains;

};
