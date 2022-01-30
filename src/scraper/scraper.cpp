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

#include "scraper.h"
#include "parser/HtmlParser.h"
#include "system/datetime.h"

using namespace std;

namespace Scraper {

	string user_agent_token() {
		return "AlexandriaBot";
	}

	string user_agent() {
		string ua_version = "1.0";
		string ua = "Mozilla/5.0 (Linux) curl (compatible; "+user_agent_token()+"/"+ua_version+"; +https://www.alexandria.org/info.html)";
		return ua;
	}

	scraper::scraper(const string &domain, store *store) :
		m_domain(domain), m_store(store)
	{
		m_domain_data.m_domain = domain;
		m_curl = nullptr;
	}

	scraper::~scraper() {
		if (m_curl) curl_easy_cleanup(m_curl);
	}

	void scraper::push_url(const URL &url) {
		m_queue.push(url);
	}

	void scraper::run() {

		download_domain_data();
		download_robots();

		while (m_queue.size()) {
			URL url = m_queue.front();
			m_queue.pop();
			cout << "url: " << url.str() << endl;
			if (robots_allow_url(url)) {
				handle_url(url);
			}
		}
	}

	void scraper::handle_error(const std::string &error) {
		
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
			char *new_url_str = nullptr;
			curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response_code);
			curl_easy_getinfo(m_curl, CURLINFO_EFFECTIVE_URL, &new_url_str);

			cout << "url: " << new_url_str << endl;

			if (new_url_str != nullptr) {
				string new_u_str(new_url_str);
				URL new_url(new_u_str);
				update_url(new_url, response_code, System::cur_datetime(), URL());
				update_url(url, 301, System::cur_datetime(), new_url); // A bit of cheeting heere, it is not sure the original url had a 301 response code.
				handle_response(m_buffer, response_code, new_url);
			} else {
				update_url(url, response_code, System::cur_datetime(), URL());
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

	void scraper::update_url(const URL &url, size_t http_code, size_t last_visited, const URL &redirect) {
		UrlStore::UrlData url_data;
		url_data.m_url = url;
		url_data.m_redirect = redirect;
		url_data.m_http_code = http_code;
		url_data.m_last_visited = last_visited;

		cout << "updating url: " << url_data.m_url.str() << endl;
		UrlStore::update(url_data, UrlStore::update_url | UrlStore::update_redirect | UrlStore::update_http_code | UrlStore::update_last_visited);
	}

	void scraper::handle_response(const string &data, size_t response_code, const URL &url) {
		HtmlParser html_parser;
		html_parser.parse(data, url.str());

		const string ip = "";
		const string date = "";

		if (html_parser.should_insert()) {
			const string line = (url.str()
				+ '\t' + html_parser.title()
				+ '\t' + html_parser.h1()
				+ '\t' + html_parser.meta()
				+ '\t' + html_parser.text()
				+ '\t' + date
				+ '\t' + ip
				+ '\n');
			m_store->add_scraper_data(line);
			string links;
			for (const auto &link : html_parser.links()) {
				links += (link.host()
					+ '\t' + link.path()
					+ '\t' + link.target_host()
					+ '\t' + link.target_path()
					+ '\t' + link.text()
					+ '\t' + (link.nofollow() ? "1" : "0")
					+ '\n');
			}
			m_store->add_link_data(links);
			m_store->upload_results();
		}
	}

	void scraper::download_domain_data() {
		int error = UrlStore::get(m_domain, m_domain_data);
		if (error == UrlStore::ERROR) {
			handle_error("could not download domain data");
		}
	}

	void scraper::download_robots() {
		const string robots_path = "http://" + m_domain + "/robots.txt";//path_to_url("/robots.txt");
		m_robots_content = simple_get(URL(robots_path));
	}

	bool scraper::robots_allow_url(const URL &url) const {
		googlebot::RobotsMatcher matcher;
		return matcher.OneAgentAllowedByRobots(m_robots_content, user_agent(), url.str());
	}

	string scraper::simple_get(const URL &url) {
		m_curl = curl_easy_init();
		curl_easy_setopt(m_curl, CURLOPT_USERAGENT, user_agent().c_str());
		curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1l);
		curl_easy_setopt(m_curl, CURLOPT_MAXREDIRS, 5l);
		curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, curl_string_reader);
		curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(m_curl, CURLOPT_URL, url.str().c_str());

		m_buffer.resize(0);
		curl_easy_perform(m_curl);

		curl_easy_cleanup(m_curl);
		m_curl = nullptr;

		return m_buffer;
	}

	size_t curl_string_reader(char *ptr, size_t size, size_t nmemb, void *userdata) {
		const size_t byte_size = size * nmemb;
		scraper *s = static_cast<scraper *>(userdata);
		if (s->m_buffer_len < s->m_buffer.size() + byte_size) return 0;
		s->m_buffer.append(ptr, byte_size);
		return byte_size;
	}

}
