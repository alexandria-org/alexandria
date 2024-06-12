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

#include "search_server.h"

#include <iostream>
#include "http/server.h"
#include "indexer/index_manager.h"
#include "indexer/url_record.h"
#include "hash_table2/hash_table.h"
#include "transfer/transfer.h"
#include "parser/parser.h"
#include "parser/unicode.h"
#include "api/result_with_snippet.h"
#include "api/api_response.h"
#include "full_text/search_metric.h"
#include "json.hpp"

namespace server {

	void search_server() {

		indexer::index_manager idx_manager;

		cout << "starting server..." << endl;

		::http::server srv([&idx_manager](const http::request &req) {
			http::response res;

			URL url = req.url();

			auto query = url.query();

			size_t limit = 1000;
			if (query.count("limit")) limit = std::stoi(query["limit"]);

			(void)limit;

			if (url.path() == "/favicon.ico") {
				res.code(404);
				res.body("404");
				return res;
			}

			stringstream body;

			// implement the same search server logic we have on alexandria.org now.
			LOG_INFO("Serving request: " + url.path());

			bool deduplicate = true;
			if (query.find("d") != query.end()) {
				if (query["d"] == "a") {
					deduplicate = false;
				}
			}

			if (query.find("q") != query.end() && deduplicate) {
				size_t total_num_results;
				auto response = idx_manager.find(total_num_results, query["q"]);
			}

			res.code(200);

			res.body(body.str());

			return res;
		});
	}
}
