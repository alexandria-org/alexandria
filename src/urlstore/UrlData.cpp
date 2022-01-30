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

#include "UrlData.h"

using namespace std;
using json = nlohmann::ordered_json;

namespace UrlStore {

	const std::string UrlData::uri = "url";

	// Internal data structure, this is what is stored in byte array together with URLs.
	struct UrlDataStore {
		size_t link_count;
		size_t http_code;
		size_t last_visited;
	};

	UrlData::UrlData() {
	}

	UrlData::UrlData(const char *cstr, size_t len) {
		if (len < sizeof(UrlDataStore) + 2*sizeof(size_t)) {
			return;
		}

		UrlDataStore data_store = *((UrlDataStore *)&cstr[0]);

		const size_t offs_url_len = sizeof(UrlDataStore);
		const size_t offs_url = offs_url_len + sizeof(size_t);
		const size_t url_len = *((size_t *)&cstr[offs_url_len]);

		const size_t offs_redirect_len = offs_url + url_len;
		if (offs_redirect_len >= len) return;

		const size_t offs_redirect = offs_redirect_len + sizeof(size_t);
		const size_t redirect_len = *((size_t *)&cstr[offs_redirect_len]);

		m_link_count = data_store.link_count;
		m_http_code = data_store.http_code;
		m_last_visited = data_store.last_visited;

		if (offs_redirect + redirect_len <= len) {
			string url_str = string(&cstr[offs_url], url_len);
			m_url = URL(url_str);
			string redirect_str = string(&cstr[offs_redirect], *((size_t *)&cstr[offs_redirect_len]));
			m_redirect = URL(redirect_str);
		}
	}

	UrlData::UrlData(const string &str) : UrlData(str.c_str(), str.size()){
	}

	UrlData::~UrlData() {
	}

	void UrlData::apply_update(const UrlData &src, size_t update_bitmask) {
		if (update_bitmask & update_url) m_url = src.m_url;
		if (update_bitmask & update_redirect) m_redirect = src.m_redirect;
		if (update_bitmask & update_link_count) m_link_count = src.m_link_count;
		if (update_bitmask & update_http_code) m_http_code = src.m_http_code;
		if (update_bitmask & update_last_visited) m_last_visited = src.m_last_visited;
	}

	string UrlData::to_str() const {

		UrlDataStore data_store = {
			.link_count = m_link_count,
			.http_code = m_http_code,
			.last_visited = m_last_visited
		};

		string str;
		str.append((char *)&data_store, sizeof(data_store));

		const size_t url_len = m_url.size();
		str.append((char *)&url_len, sizeof(url_len));
		str.append(m_url.str());

		const size_t redirect_len = m_redirect.size();
		str.append((char *)&redirect_len, sizeof(redirect_len));
		str.append(m_redirect.str());

		return str;
	}

	string UrlData::private_key() const {
		return m_url.key();
	}

	string UrlData::public_key() const {
		return m_url.str();
	}

	json UrlData::to_json() const {
		json message;
		message["url"] = m_url.str();
		message["redirect"] = m_redirect.str();
		message["link_count"] = m_link_count;
		message["http_code"] = m_http_code;
		message["last_visited"] = m_last_visited;

		return message;
	}

}
