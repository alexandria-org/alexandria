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

class URL {

public:
	URL();
	explicit URL(const std::string &url);
	explicit URL(const std::string &host, const std::string &path);
	~URL();

	static std::string host_reverse(const std::string &host);
	static std::string host_reverse_top_domain(const std::string &host);

	void set_url_string(const std::string &url);
	std::string str() const;

	uint64_t hash() const;
	uint64_t host_hash() const;
	uint64_t link_hash(const URL &target_url, const std::string &link_text) const;
	uint64_t domain_link_hash(const URL &target_url, const std::string &link_text) const;

	std::string host() const;
	std::string host_top_domain() const;
	std::string scheme() const;
	std::string path() const;
	std::string path_with_query() const;
	std::map<std::string, std::string> query() const;
	std::string host_reverse() const;
	std::string domain_without_tld() const;
	uint32_t size() const;

	float harmonic(const SubSystem *sub_system) const;

	friend std::istream &operator >>(std::istream &ss, URL &url);
	friend std::ostream &operator <<(std::ostream& os, const URL& url);

private:

	std::hash<std::string> m_hasher;
	std::string m_url_string;
	std::string m_host;
	std::string m_host_reverse;
	std::string m_scheme;
	std::string m_path;
	std::string m_query;
	int m_status;

	int parse();
	inline void remove_www(std::string &path);


};
