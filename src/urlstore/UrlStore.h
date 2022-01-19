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
#include <vector>
#include <bitset>
#include "KeyValueStore.h"
#include "parser/URL.h"
#include "leveldb/db.h"
#include "leveldb/write_batch.h"

#define URL_STORE_NUM_FIELDS 4

namespace UrlStore {

	const int OK = 0;
	const int ERROR = 1;

	const size_t update_redirect     = 0B00000001;
	const size_t update_link_count   = 0B00000010;
	const size_t update_http_code    = 0B00000100;
	const size_t update_last_visited = 0B00001000;

	struct UrlData {
		URL url;
		URL redirect;
		size_t link_count;
		size_t http_code;
		size_t last_visited;
	};

	std::string data_to_str(const UrlData &data);
	UrlData str_to_data(const char *cstr, size_t len);
	UrlData str_to_data(const std::string &str);

	class UrlStore {
		public:
			UrlStore(const std::string &server_path);
			~UrlStore();

			void set(const UrlData &data);
			UrlData get(const URL &url);
			leveldb::DB *db() { return m_db.db(); }

		private:
			KeyValueStore m_db;


	};

	void print_url_data(const UrlData &data);

	void handle_put_request(UrlStore &store, const std::string &post_data, std::stringstream &response_stream);
	void handle_binary_get_request(UrlStore &store, const URL &url, std::stringstream &response_stream);
	void handle_get_request(UrlStore &store, const URL &url, std::stringstream &response_stream);

	void set(const std::vector<UrlData> &data);
	void set(const UrlData &data);
	void update(const std::vector<UrlData> &data, size_t update_bitmask);
	void update(const UrlData &data, size_t update_bitmask);
	int get(const URL &url, UrlData &data);


}
