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
#include <fstream>
#include <unordered_map>

class UrlToDomain {

public:
	explicit UrlToDomain(const std::string &db_name);
	~UrlToDomain();

	void add_url(uint64_t url_hash, uint64_t domain_hash);
	void read();
	void write(size_t indexer_id);
	void truncate();

	size_t size() const {
		return m_url_to_domain.size();
	}

	bool has_url(uint64_t url_hash) {
		return m_url_to_domain.count(url_hash) > 0;
	}

	bool has_domain(uint64_t domain_hash) {
		return m_domains.count(domain_hash) > 0;
	}


	const std::unordered_map<uint64_t, uint64_t> &url_to_domain() const { return m_url_to_domain; };
	const std::unordered_map<uint64_t, size_t> &domains() const { return m_domains; };

private:
	const std::string m_db_name;
	std::unordered_map<uint64_t, uint64_t> m_url_to_domain;
	std::unordered_map<uint64_t, size_t> m_domains;

};
