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

using namespace std;

namespace UrlStore {

	UrlStore::UrlStore(const string &server_path) {
		leveldb::Options options;
		options.create_if_missing = true;
		leveldb::Status status = leveldb::DB::Open(options, server_path, &m_db);
	}

	UrlStore::~UrlStore() {
		delete m_db;
	}

	void UrlStore::set(const URL &url, const UrlData &data) {
		leveldb::Slice key = url.key();
		string str = data_to_str(data);
		leveldb::Status s = m_db->Put(leveldb::WriteOptions(), key, str);
		if (!s.ok()) {
			cerr << s.ToString() << endl;
		}
	}

	UrlData UrlStore::get(const URL &url) {
		leveldb::Slice key = url.key();
		string value;
		leveldb::Status s = m_db->Get(leveldb::ReadOptions(), key, &value);

		if (s.ok()) {
			return str_to_data(value);
		} else {
			cerr << s.ToString() << endl;
		}
		return UrlData{};
	}

	string UrlStore::data_to_str(const UrlData &data) const {
		stringstream ss;
		ss << data.url << '\t';
		ss << data.link_count << '\t';
		ss << data.http_code << '\t';
		ss << data.location << '\t';
		ss << data.last_visited;
		return ss.str();
	}

	UrlData UrlStore::str_to_data(const string &str) const {
		vector<string> parts;
		boost::algorithm::split(parts, str, boost::is_any_of("\t"));
		UrlData data = {
			.url = parts[0],
			.link_count = stoull(parts[1]),
			.http_code = stoull(parts[2]),
			.location = parts[3],
			.last_visited = stoull(parts[4])
		};
		return data;
	}

	void print_url_data(const UrlData &data) {
		cout << "url: " << data.url << endl;
		cout << "link_count: " << data.link_count << endl;
		cout << "http_code: " << data.http_code << endl;
		cout << "location: " << data.location << endl;
		cout << "last_visited: " << data.last_visited << endl;
	}
}
