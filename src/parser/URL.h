/*
 * MIT License
 *
 * Alexandria.org
 *
 * Copyright (c) 2021 Josef Cullhed, <info@alexandria.org>, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <iostream>
#include <functional>
#include <boost/algorithm/string/join.hpp>
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
