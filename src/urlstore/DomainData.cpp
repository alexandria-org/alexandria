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

#include "DomainData.h"

using namespace std;
using json = nlohmann::ordered_json;

namespace UrlStore {

	const std::string DomainData::uri = "domain";

	// Internal data structure, this is what is stored in byte array together with URLs.
	struct DomainDataStore {
		size_t has_https;
		size_t has_www;
	};

	DomainData::DomainData() {
	}

	DomainData::DomainData(const char *cstr, size_t len) {
		if (len < sizeof(DomainDataStore) + sizeof(size_t)) {
			return;
		}

		DomainDataStore data_store = *((DomainDataStore *)&cstr[0]);

		const size_t offs_domain_len = sizeof(DomainDataStore);
		const size_t offs_domain = offs_domain_len + sizeof(size_t);
		const size_t domain_len = *((size_t *)&cstr[offs_domain_len]);

		m_has_https = data_store.has_https;
		m_has_www = data_store.has_www;

		if (offs_domain + domain_len <= len) {
			m_domain = string(&cstr[offs_domain], domain_len);
		}
	}

	DomainData::DomainData(const string &str) : DomainData(str.c_str(), str.size()){
	}

	DomainData::~DomainData() {
	}

	void DomainData::apply_update(const DomainData &src, size_t update_bitmask) {
		if (update_bitmask & update_has_https) m_has_https = src.m_has_https;
		if (update_bitmask & update_has_www) m_has_www = src.m_has_www;
	}

	string DomainData::to_str() const {

		DomainDataStore data_store = {
			.has_https = m_has_https,
			.has_www = m_has_www
		};

		string str;
		str.append((char *)&data_store, sizeof(data_store));

		const size_t domain_len = m_domain.size();
		str.append((char *)&domain_len, sizeof(domain_len));
		str.append(m_domain);

		return str;
	}

	string DomainData::private_key() const {
		return m_domain;
	}

	string DomainData::public_key() const {
		return m_domain;
	}

	json DomainData::to_json() const {
		json message;
		message["domain"] = m_domain;
		message["has_https"] = m_has_https;
		message["has_www"] = m_has_www;

		return message;
	}

	std::ostream &operator<<(std::ostream &os, DomainData const &data) {
		os << data.to_json().dump(4);
		return os;
	}

}
