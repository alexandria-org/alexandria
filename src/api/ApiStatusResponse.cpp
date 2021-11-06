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

#include "ApiStatusResponse.h"
#include <aws/core/utils/json/JsonSerializer.h>
#include <system/Profiler.h>

using namespace std;

ApiStatusResponse::ApiStatusResponse(Worker::Status &status) {

	Aws::Utils::Json::JsonValue message("{}");

	Aws::Utils::Json::JsonValue string;
	Aws::Utils::Json::JsonValue json_number;

	message.WithObject("status", string.AsString("indexing"));
	message.WithObject("progress", json_number.AsDouble((double)status.items_indexed / status.items));
	message.WithObject("items_indexed", json_number.AsInt64(status.items_indexed));

	double time_left = 0.0;
	if (status.items_indexed > 0) {
		time_left = (double)status.items * status.items_indexed/((double)(Profiler::timestamp() - status.start_time));
	}
	message.WithObject("time_left", json_number.AsDouble(time_left));

	//m_response = message.View().WriteCompact();
	m_response = message.View().WriteReadable();
}

ApiStatusResponse::~ApiStatusResponse() {

}

ostream &operator<<(ostream &os, const ApiStatusResponse &api_response) {
	os << api_response.m_response;
	return os;
}

