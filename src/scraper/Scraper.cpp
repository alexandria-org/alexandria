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

#include "Scraper.h"
#include "parser/HtmlParser.h"

using namespace std;

namespace Scraper {

	string user_agent_token() {
		return "AlexandriaBot";
	}

	string user_agent() {
		string ua_version = "1.0";
		string ua = "Mozilla/5.0 (Linux) Curl (compatible; "+user_agent_token()+"/"+ua_version+"; +https://www.alexandria.org/info.html)";
		return ua;
	}

	scraper::scraper(const string &domain, store *store) :
		m_domain(domain), m_store(store)
	{
		m_curl = nullptr;
	}

	scraper::~scraper() {
		if (m_curl) curl_easy_cleanup(m_curl);
	}

	void scraper::push_url(const URL &url) {
		m_queue.push(url);
	}

	void scraper::run() {
		while (m_queue.size()) {
			URL url = m_queue.front();
			m_queue.pop();
			handle_url(url);
		}
	}

	void scraper::handle_url(const URL &url) {
		m_curl = curl_easy_init();
		curl_easy_setopt(m_curl, CURLOPT_USERAGENT, user_agent().c_str());
		curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1l);
		curl_easy_setopt(m_curl, CURLOPT_MAXREDIRS, 5l);
		curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, curl_string_reader);
		curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(m_curl, CURLOPT_URL, url.str().c_str());

		CURLcode res = curl_easy_perform(m_curl);

		if (res == CURLE_OK) {
			long response_code;
			char *new_url = nullptr;
			curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response_code);
			curl_easy_getinfo(m_curl, CURLINFO_EFFECTIVE_URL, &new_url);

			if (new_url != nullptr) {
				handle_response(m_buffer, response_code, URL(string(new_url)));
			} else {
				handle_response(m_buffer, response_code, url);
			}
		} else {
			/*
			 * Handle everything here: https://curl.se/libcurl/c/libcurl-errors.html
			 * */
			/*CURLE_UNSUPPORTED_PROTOCOL
			CURLE_FAILED_INIT
			CURLE_URL_MALFORMAT
			CURLE_NOT_BUILT_IN
			CURLE_COULDNT_RESOLVE_PROXY
			CURLE_COULDNT_RESOLVE_HOST
			CURLE_COULDNT_CONNECT
			CURLE_WEIRD_SERVER_REPLY
			CURLE_REMOTE_ACCESS_DENIED
			CURLE_HTTP2
			CURLE_PARTIAL_FILE
			CURLE_WRITE_ERROR
			CURLE_OPERATION_TIMEDOUT
			CURLE_SSL_CONNECT_ERROR
			CURLE_FUNCTION_NOT_FOUND
			CURLE_ABORTED_BY_CALLBACK
			CURLE_BAD_FUNCTION_ARGUMENT
			CURLE_INTERFACE_FAILED
			CURLE_TOO_MANY_REDIRECTS
			CURLE_GOT_NOTHING
			CURLE_SSL_ENGINE_NOTFOUND
			CURLE_SSL_ENGINE_SETFAILED
			CURLE_SEND_ERROR
			CURLE_RECV_ERROR
			CURLE_SSL_CERTPROBLEM
			CURLE_SSL_CIPHER
			CURLE_PEER_FAILED_VERIFICATION
			CURLE_BAD_CONTENT_ENCODING
			CURLE_SSL_ENGINE_INITFAILED
			CURLE_SSL_CACERT_BADFILE
			CURLE_SSL_CRL_BADFILE
			CURLE_SSL_ISSUER_ERROR
			CURLE_HTTP3
			CURLE_SSL_CLIENTCERT*/
		}

		m_buffer.resize(0);
		m_buffer.shrink_to_fit();
		curl_easy_cleanup(m_curl);
		m_curl = nullptr;
	}

	void scraper::handle_response(const string &data, size_t response_code, const URL &url) {
		HtmlParser parser;
		parser.parse(data);
	}

	size_t curl_string_reader(char *ptr, size_t size, size_t nmemb, void *userdata) {
		const size_t byte_size = size * nmemb;
		scraper *s = static_cast<scraper *>(userdata);
		if (s->m_buffer_len < s->m_buffer.size() + byte_size) return 0;
		s->m_buffer.append(ptr, byte_size);
		return byte_size;
	}

}
