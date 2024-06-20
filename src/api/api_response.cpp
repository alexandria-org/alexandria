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
#include "indexer/return_record.h"
#include "full_text/search_metric.h"
#include "parser/unicode.h"
#include "json.hpp"

namespace api {

	api_response::api_response(const std::vector<indexer::return_record> &results, const struct full_text::search_metric &metric, double profile) {

		using json = nlohmann::ordered_json;

		json message;

		json result_array;
		for (const auto &result : results) {
			json json_result;

			try {
				json_result["url"] = result.m_url.str();
				json_result["title"] = parser::unicode::encode(result.m_title);
				json_result["snippet"] = parser::unicode::encode(result.m_snippet);
				json_result["score"] = result.m_score;
				json_result["domain_hash"] = std::to_string(result.m_domain_hash);
				json_result["url_hash"] = std::to_string(result.m_url.hash());

				result_array.push_back(json_result);
			} catch (nlohmann::detail::type_error &error) {
				// skip this result.
				// in future log this and fix what is wrong.
			}
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

	std::ostream &operator<<(std::ostream &os, const api_response &api_response) {
		os << api_response.m_response;
		return os;
	}

}
