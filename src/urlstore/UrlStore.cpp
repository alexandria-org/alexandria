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

#include "UrlStore.h"
#include "config.h"
#include "transfer/Transfer.h"
#include "parser/URL.h"

using namespace std;

namespace UrlStore {

	// Internal data structure, this is what is stored in byte array together with URLs.
	struct UrlDataStore {
		size_t link_count;
		size_t http_code;
		size_t last_visited;
	};

	string data_to_str(const UrlData &data) {

		UrlDataStore data_store = {
			.link_count = data.link_count,
			.http_code = data.http_code,
			.last_visited = data.last_visited
		};

		string str;
		str.append((char *)&data_store, sizeof(data_store));

		const size_t url_len = data.url.size();
		str.append((char *)&url_len, sizeof(url_len));
		str.append(data.url.str());

		const size_t redirect_len = data.redirect.size();
		str.append((char *)&redirect_len, sizeof(redirect_len));
		str.append(data.redirect.str());

		return str;
	}

	UrlData str_to_data(const char *cstr, size_t len) {

		if (len < sizeof(UrlDataStore) + 2*sizeof(size_t)) {
			return UrlData{};
		}

		UrlDataStore data_store = *((UrlDataStore *)&cstr[0]);

		const size_t offs_url_len = sizeof(UrlDataStore);
		const size_t offs_url = offs_url_len + sizeof(size_t);
		const size_t url_len = *((size_t *)&cstr[offs_url_len]);

		const size_t offs_redirect_len = offs_url + url_len;
		if (offs_redirect_len >= len) return UrlData{};

		const size_t offs_redirect = offs_redirect_len + sizeof(size_t);
		const size_t redirect_len = *((size_t *)&cstr[offs_redirect_len]);

		UrlData url_data = {
			.link_count = data_store.link_count,
			.http_code = data_store.http_code,
			.last_visited = data_store.last_visited
		};

		if (offs_redirect + redirect_len <= len) {
			string url_str = string(&cstr[offs_url], url_len);
			url_data.url = URL(url_str);
			string redirect_str = string(&cstr[offs_redirect], *((size_t *)&cstr[offs_redirect_len]));
			url_data.redirect = URL(redirect_str);
		}

		return url_data;
	}

	UrlData str_to_data(const string &str) {
		return str_to_data(str.c_str(), str.size());
	}

	UrlStore::UrlStore(const string &server_path) {
		leveldb::Options options;
		options.create_if_missing = true;
		leveldb::Status status = leveldb::DB::Open(options, server_path, &m_db);
	}

	UrlStore::~UrlStore() {
		delete m_db;
	}

	void UrlStore::set(const UrlData &data) {
		leveldb::Slice key = data.url.key();
		string str = data_to_str(data);
		cout << "setting data at key: " << data.url.key() << endl;
		leveldb::Status s = m_db->Put(leveldb::WriteOptions(), key, str);
		if (!s.ok()) {
			cerr << s.ToString() << endl;
		}
	}

	UrlData UrlStore::get(const URL &url) {
		leveldb::Slice key = url.key();
		string value;
		cout << "fetching key: " << url.key() << endl;
		leveldb::Status s = m_db->Get(leveldb::ReadOptions(), key, &value);

		if (s.ok()) {
			return str_to_data(value);
		} else {
			cerr << s.ToString() << endl;
		}
		return UrlData{};
	}

	void print_binary_url_data(const UrlData &data, ostream &stream) {
		stream << data_to_str(data);
	}

	void print_url_data(const UrlData &data, ostream &stream) {
		stream << "url: " << data.url << endl;
		stream << "redirect: " << data.redirect << endl;
		stream << "link_count: " << data.link_count << endl;
		stream << "http_code: " << data.http_code << endl;
		stream << "last_visited: " << data.last_visited << endl;
	}

	void print_url_data(const UrlData &data) {
		return print_url_data(data, cout);
	}

	void handle_put_request(UrlStore &store, const std::string &post_data, std::stringstream &response_stream) {

		const char *cstr = post_data.c_str();
		const size_t len = post_data.size();

		size_t iter = 0;
		while (iter < len) {
			size_t data_len = *((size_t *)&cstr[iter]);
			iter += sizeof(size_t);

			if (data_len + iter > len) break;

			UrlData data = str_to_data(&cstr[iter], data_len);
			store.set(data);

			iter += data_len;
		}
	}

	void handle_binary_get_request(UrlStore &store, const URL &url, std::stringstream &response_stream) {
		UrlData data = store.get(url);
		cout << "GOT SOME DATA" << endl;
		print_binary_url_data(data, response_stream);
	}

	void handle_get_request(UrlStore &store, const URL &url, std::stringstream &response_stream) {
		UrlData data = store.get(url);
		cout << "GOT SOME DATA" << endl;
		print_url_data(data, response_stream);
	}

	void append_data_str(const UrlData &data, string &append_to) {
		string data_str = data_to_str(data);
		size_t data_len = data_str.size();
		append_to.append((char *)&data_len, sizeof(size_t));
		append_to.append(data_str);
	}

	void set(const vector<UrlData> &datas) {
		string put_data;
		for (const UrlData &data : datas) {
			append_data_str(data, put_data);
		}
		Transfer::put(Config::url_store_host + "/urlstore", put_data);
	}

	void set(const UrlData &data) {
		string put_data;
		append_data_str(data, put_data);
		Transfer::put(Config::url_store_host + "/urlstore", put_data);
	}

	int get(const URL &url, UrlData &data) {
		Transfer::Response res = Transfer::get(Config::url_store_host + "/urlstore/" + url.str(), {"Accept: application/octet-stream"});
		if (res.code == 200) {
			data = str_to_data(res.body);
			return OK;
		}
		return ERROR;
	}
}
