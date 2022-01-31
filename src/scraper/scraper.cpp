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
#include "text/Text.h"
#include <memory>

using namespace std;

namespace Scraper {

	string user_agent_token() {
		return "AlexandriaBot";
	}

	string user_agent() {
		string ua_version = "1.0";
		string ua = "Mozilla/5.0 (Linux) (compatible; "+user_agent_token()+"/"+ua_version+"; +https://www.alexandria.org/info.html)";
		return ua;
	}

	scraper::scraper(const string &domain, store *store) :
		m_domain(domain), m_store(store)
	{
		m_domain_data.m_domain = domain;
		m_curl = nullptr;
	}

	scraper::~scraper() {
		m_thread.join();
		upload_domain_info();
		if (m_curl) curl_easy_cleanup(m_curl);
	}

	void scraper::push_url(const URL &url) {
		m_queue.push(url);
	}

	void scraper::run() {

		download_domain_data();
		download_robots();

		while (m_queue.size()) {
			URL url = filter_url(m_queue.front());
			m_queue.pop();
			if (robots_allow_url(url)) {
				handle_url(url);
				this_thread::sleep_for(300s);
			}
		}

		m_finished = true;
	}

	void scraper::handle_error(const std::string &error) {
		
	}

	void scraper::handle_url(const URL &url) {
		m_buffer.resize(0);
		m_curl = curl_easy_init();
		LOG_INFO(m_domain + ": scraping url: " + url.str());
		curl_easy_setopt(m_curl, CURLOPT_USERAGENT, user_agent().c_str());
		curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1l);
		curl_easy_setopt(m_curl, CURLOPT_MAXREDIRS, 5l);
		curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, curl_string_reader);
		curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(m_curl, CURLOPT_URL, url.str().c_str());
		curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, 30);

		CURLcode res = curl_easy_perform(m_curl);

		if (res == CURLE_OK) {
			long response_code;
			char *new_url_str = nullptr;
			curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response_code);
			curl_easy_getinfo(m_curl, CURLINFO_EFFECTIVE_URL, &new_url_str);

			// Fetch IP address.
			char *ip_cstr;
			string ip;
			if (!curl_easy_getinfo(m_curl, CURLINFO_PRIMARY_IP, &ip_cstr) && ip_cstr != nullptr) ip = string(ip_cstr);

			if (new_url_str != nullptr) {
				string new_u_str(new_url_str);
				URL new_url(new_u_str);
				LOG_INFO(m_domain + ": redirected to: " + new_url.str());
				update_url(new_url, response_code, System::cur_datetime(), URL());
				if (url.canonically_different(new_url)) {
					update_url(url, 301, System::cur_datetime(), new_url); // A bit of cheeting heere, it is not sure the original url had a 301 response code.
				}
				handle_response(m_buffer, response_code, ip, new_url);
			} else {
				update_url(url, response_code, System::cur_datetime(), URL());
				handle_response(m_buffer, response_code, ip, url);
			}
		} else {
			LOG_INFO(m_domain + " got error code: " +to_string(res)+ " for url: " + url.str());
			/*
			 * Handle everything here: https://curl.se/libcurl/c/libcurl-errors.html
			 * */
			vector<CURLcode> domain_errors = {
				CURLE_COULDNT_RESOLVE_HOST,
				CURLE_COULDNT_CONNECT,
			};

			if (res == CURLE_COULDNT_RESOLVE_HOST || res == CURLE_COULDNT_CONNECT) {
				update_url(url, 10000 + res, System::cur_datetime(), URL());
				mark_all_urls_with_error(10000 + res);
			} else {
				update_url(url, 10000 + res, System::cur_datetime(), URL());
			}
		}

		m_buffer.resize(0);
		m_buffer.shrink_to_fit();
		curl_easy_cleanup(m_curl);
		m_curl = nullptr;
	}

	void scraper::mark_all_urls_with_error(size_t error_code) {
		while (m_queue.size()) {
			URL url = filter_url(m_queue.front());
			m_queue.pop();
			update_url(url, error_code, System::cur_datetime(), URL());
		}
	}

	void scraper::update_url(const URL &url, size_t http_code, size_t last_visited, const URL &redirect) {
		UrlStore::UrlData url_data;
		url_data.m_url = url;
		url_data.m_redirect = redirect;
		url_data.m_http_code = http_code;
		url_data.m_last_visited = last_visited;

		m_store->add_url_data(url_data);
	}

	void scraper::handle_response(const string &data, size_t response_code, const string &ip, const URL &url) {
		LOG_INFO(m_domain + ": storing response for " + url.str());
		HtmlParser html_parser;
		html_parser.parse(data, url.str());

		if (response_code == 200) {
			m_num_total++;
			if (url.has_www()) m_num_www++; 
			if (url.has_https()) m_num_https++; 
			if (m_num_total == 3) upload_domain_info();
		}

		const string date = System::iso8601_datetime();

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
		const URL robots_path = filter_url(URL("http://" + m_domain + "/robots.txt"));
		m_robots_content = simple_get(robots_path);
	}

	bool scraper::robots_allow_url(const URL &url) const {
		googlebot::RobotsMatcher matcher;
		bool allowed = matcher.OneAgentAllowedByRobots(m_robots_content, user_agent_token(), url.str());
		return allowed;
	}

	string scraper::simple_get(const URL &url) {
		m_curl = curl_easy_init();
		curl_easy_setopt(m_curl, CURLOPT_USERAGENT, user_agent().c_str());
		curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1l);
		curl_easy_setopt(m_curl, CURLOPT_MAXREDIRS, 5l);
		curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, curl_string_reader);
		curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(m_curl, CURLOPT_URL, url.str().c_str());
		curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, 30);

		m_buffer.resize(0);
		curl_easy_perform(m_curl);

		curl_easy_cleanup(m_curl);
		m_curl = nullptr;

		return m_buffer;
	}

	void scraper::upload_domain_info() {
		if (m_num_total > 0) {
			UrlStore::DomainData data;
			data.m_domain = m_domain;
			data.m_has_https = m_num_https > 0;
			data.m_has_www = m_num_www > 0;

			UrlStore::set(data);
		}
	}

	URL scraper::filter_url(const URL &url) {
		URL ret(url);
		if (m_domain_data.m_has_https && !url.has_https()) ret.set_scheme("https");
		if (m_domain_data.m_has_www && !url.has_www()) ret.set_www(true);

		return ret;
	}

	void scraper::start_thread() {
		m_thread = std::move(thread([this](){
			this->run();
		}));
	}

	bool scraper::finished() {
		return m_finished;
	}

	size_t curl_string_reader(char *ptr, size_t size, size_t nmemb, void *userdata) {
		const size_t byte_size = size * nmemb;
		scraper *s = static_cast<scraper *>(userdata);
		if (s->m_buffer_len < s->m_buffer.size() + byte_size) return 0;
		s->m_buffer.append(ptr, byte_size);
		return byte_size;
	}

	bool reset_scraper_urls() {
		string content = "";
		int error = Transfer::upload_file("nodes/" + Config::node + "/scraper.urls", content);
		return error == Transfer::OK;
	}

	vector<string> download_scraper_urls() {
		int error;
		string content = Transfer::file_to_string("nodes/" + Config::node + "/scraper.urls", error);
		if (error == Transfer::ERROR) return {};

		reset_scraper_urls();

		content = Text::trim(content);

		vector<string> urls;
		boost::algorithm::split(urls, content, boost::is_any_of("\n"));

		return urls;
	}

	void run_scraper_on_urls(const vector<string> &input_urls) {
		const size_t max_scrapers = 10;
		Scraper::store store;
		map<string, unique_ptr<scraper>> scrapers;

		vector<string> urls = input_urls;
		while (urls.size()) {

			vector<string> unhandled_urls;

			for (const string &url_str : urls) {
				URL url(url_str);

				if (!scrapers.count(url.host())) {
					if (scrapers.size() >= max_scrapers) {
						unhandled_urls.push_back(url_str);
					} else {
						scrapers[url.host()] = make_unique<scraper>(url.host(), &store);
						scrapers[url.host()]->push_url(url);
					}
				} else {
					scrapers[url.host()]->push_url(url);
				}
			}
			// Start scrapers.
			for (auto &iter : scrapers) {
				iter.second->start_thread();
			}
			
			// Wait for at least 50% of the scrapers to finish.
			while (scrapers.size() > max_scrapers * 0.5) {
				for (auto &iter : scrapers) {
					if (iter.second->finished()) {
						scrapers.erase(iter.first);
					}
				}
				this_thread::sleep_for(1000ms);
			}
			urls = unhandled_urls;
		}
	}

	void url_downloader() {

		const size_t timeout = 300;
		//const size_t limit = 500;

		// main loop
		while (true) {

			// Check if there are any urls to digest every 'timeout' minutes.
			vector<string> urls = download_scraper_urls();

			if (urls.size() > 0) {
				run_scraper_on_urls(urls);
			}

			sleep(timeout);
		}
	}

}
