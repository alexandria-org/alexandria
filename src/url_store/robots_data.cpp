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

#include "robots_data.h"

using namespace std;
using json = nlohmann::ordered_json;

namespace url_store {

	const std::string robots_data::uri = "robots";

	robots_data::robots_data() {
	}

	robots_data::robots_data(const char *cstr, size_t len) {
		if (len < 2*sizeof(size_t)) {
			return;
		}

		const size_t offs_domain_len = 0;
		const size_t offs_domain = offs_domain_len + sizeof(size_t);
		const size_t domain_len = *((size_t *)&cstr[offs_domain_len]);

		const size_t offs_robots_len = offs_domain + domain_len;
		const size_t offs_robots = offs_robots_len + sizeof(size_t);

		if (offs_robots_len < len) {
			const size_t robots_len = *((size_t *)&cstr[offs_robots_len]);

			if (offs_robots + robots_len <= len) {
				m_domain = string(&cstr[offs_domain], domain_len);
				m_robots = string(&cstr[offs_robots], robots_len);
			}
		}
	}

	robots_data::robots_data(const string &str) : robots_data(str.c_str(), str.size()){
	}

	robots_data::~robots_data() {
	}

	void robots_data::apply_update(const robots_data &src, size_t update_bitmask) {
		if (update_bitmask & update_robots) m_robots = src.m_robots;
	}

	string robots_data::to_str() const {

		string str;

		const size_t domain_len = m_domain.size();
		str.append((char *)&domain_len, sizeof(domain_len));
		str.append(m_domain);

		const size_t robots_len = m_robots.size();
		str.append((char *)&robots_len, sizeof(robots_len));
		str.append(m_robots);

		return str;
	}

	string robots_data::private_key() const {
		return m_domain;
	}

	string robots_data::public_key() const {
		return m_domain;
	}

	json robots_data::to_json() const {
		json message;
		message["domain"] = m_domain;
		message["robots"] = m_robots;

		return message;
	}

}
