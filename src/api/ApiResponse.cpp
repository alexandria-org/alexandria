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

#include "ApiResponse.h"
#include "parser/Unicode.h"

using namespace Aws::Utils::Json;

ApiResponse::ApiResponse(vector<ResultWithSnippet> &results, const struct SearchMetric &metric, double profile) {

	JsonValue message("{}");

	Aws::Utils::Array<Aws::Utils::Json::JsonValue> result_array(results.size());

	size_t idx = 0;
	for (const ResultWithSnippet &result : results) {
		JsonValue json_result;
		JsonValue string;
		JsonValue json_number;

		//cout << result.title() << endl;
		//cout << result.snippet() << endl;

		json_result.WithObject("url", string.AsString(result.url().str()));
		json_result.WithObject("title", string.AsString(Unicode::encode(result.title())));
		json_result.WithObject("snippet", string.AsString(Unicode::encode(result.snippet())));
		json_result.WithObject("score", json_number.AsDouble(result.score()));
		json_result.WithObject("domain_hash", string.AsString(to_string(result.domain_hash())));
		json_result.WithObject("url_hash", string.AsString(to_string(result.url().hash())));
		result_array[idx] = json_result;
		idx++;
	}

	JsonValue json_results, json_string, json_number;
	json_results.AsArray(result_array);
	message.WithObject("status", json_string.AsString("success"));
	message.WithObject("time_ms", json_string.AsDouble(profile));
	message.WithObject("total_found", json_number.AsInt64(metric.m_total_found));
	message.WithObject("total_links_found", json_number.AsInt64(metric.m_total_links_found));
	message.WithObject("links_handled", json_number.AsInt64(metric.m_links_handled));
	message.WithObject("link_domain_matches", json_number.AsInt64(metric.m_link_domain_matches));
	message.WithObject("link_url_matches", json_number.AsInt64(metric.m_link_url_matches));
	message.WithObject("results", json_results);

	//m_response = message.View().WriteCompact();
	m_response = message.View().WriteReadable();
}

ApiResponse::~ApiResponse() {

}

ostream &operator<<(ostream &os, const ApiResponse &api_response) {
	os << api_response.m_response;
	return os;
}

