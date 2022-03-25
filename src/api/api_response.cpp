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

#include "api_response.h"
#include "api/result_with_snippet.h"
#include "full_text/SearchMetric.h"
#include "parser/Unicode.h"
#include "json.hpp"

using namespace std;
using json = nlohmann::ordered_json;

namespace api {

	api_response::api_response(vector<result_with_snippet> &results, const struct SearchMetric &metric, double profile) {

		json message;

		json result_array;
		for (const result_with_snippet &result : results) {
			json json_result;

			json_result["url"] = result.url().str();
			json_result["title"] = Unicode::encode(result.title());
			json_result["snippet"] = Unicode::encode(result.snippet());
			json_result["score"] = result.score();
			json_result["domain_hash"] = to_string(result.domain_hash());
			json_result["url_hash"] = to_string(result.url().hash());

			result_array.push_back(json_result);
		}

		message["status"] = "success";
		message["time_ms"] = profile;
		message["total_found"] = metric.m_total_found;
		message["total_url_links_found"] = metric.m_total_url_links_found;
		message["total_domain_links_found"] = metric.m_total_domain_links_found;
		message["links_handled"] = metric.m_links_handled;
		message["link_domain_matches"] = metric.m_link_domain_matches;
		message["link_url_matches"] = metric.m_link_url_matches;
		message["results"] = result_array;

		//m_response = message.dump();
		m_response = message.dump(4);
	}

	api_response::~api_response() {

	}

	ostream &operator<<(ostream &os, const api_response &api_response) {
		os << api_response.m_response;
		return os;
	}

}
