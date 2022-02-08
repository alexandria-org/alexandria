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
		return "AlexandriaOrgBot";
	}

	string user_agent() {
		string ua_version = "1.0";
		string ua = "Mozilla/5.0 (Linux) (compatible; "+user_agent_token()+"/"+ua_version+"; +https://www.alexandria.org/bot.html)";
		return ua;
	}

	stats::stats() {
	}

	stats::~stats() {
		m_running = false;
		if (m_thread.joinable()) m_thread.join();
	}

	void stats::start_thread(size_t timeout) {
		m_timeout = timeout;
		m_thread = std::move(thread([this]() {
			this->run();
		}));
	}

	void stats::start_count(size_t urls_in_queue) {
		m_unfinished_scrapers = 0;
		m_unfinished_scraped_urls = 0;
		m_unfinished_scraped_urls_non200 = 0;
		m_unfinished_scraped_errors = 0;
		m_urls_in_queue = urls_in_queue;
		m_urls_assigned = 0;
	}

	void stats::count_finished(const scraper &scraper) {
		m_scraped_urls += scraper.num_scraped();
		m_scraped_urls_non200 += scraper.num_scraped_non200();
		m_scraped_errors += scraper.num_errors();
		m_finished_scrapers += 1;
		m_num_blocked += scraper.blocked() ? 1 : 0;
	}

	void stats::count_unfinished(const scraper &scraper) {
		m_unfinished_scraped_urls += scraper.num_scraped();
		m_unfinished_scraped_urls_non200 += scraper.num_scraped_non200();
		m_unfinished_scraped_errors += scraper.num_errors();
		m_unfinished_scrapers += 1;
		m_urls_assigned += scraper.size();
	}

	void stats::run() {
		size_t time_start = Profiler::timestamp();
		while (m_running) {
			std::this_thread::sleep_for(std::chrono::seconds(m_timeout));
			log_report(Profiler::timestamp() - time_start);
		}
	}

	void stats::log_report(size_t dt) {
		std::stringstream ss;
		ss.precision(2);
		ss << endl;
		ss << "Scraper stats:" << endl;
		ss << m_urls_in_queue << " urls in queue (not assigned to any scraper)" << endl;
		ss << m_urls_assigned << " urls assigned to running scrapers" << endl;
		ss << (m_scraped_urls + m_unfinished_scraped_urls) << " urls done (200 response)" << endl;
		ss << (m_scraped_urls_non200 + m_unfinished_scraped_urls_non200) << " urls (non 200 response)" << endl;
		ss << (m_scraped_errors + m_unfinished_scraped_errors) << " urls (errors)" << endl;
		ss << fixed << (double)(m_scraped_urls + m_unfinished_scraped_urls)/dt << "/s" << endl;
		ss << m_finished_scrapers << " finished scrapers" << endl;
		ss << m_unfinished_scrapers << " unfinished scrapers" << endl;
		ss << m_num_blocked << " blocked scrapers" << endl;
		LOG_INFO(ss.str());
	}

	scraper::scraper(const string &domain, store *store) :
		m_domain(domain), m_store(store)
	{
		m_domain_data.m_domain = domain;
		m_curl = curl_easy_init();
	}

	scraper::~scraper() {
		if (m_thread.joinable()) m_thread.join();
		upload_domain_info();
		curl_easy_cleanup(m_curl);
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
				if (m_timeout) {
					this_thread::sleep_for(std::chrono::seconds(m_timeout/2 + (rand() % m_timeout)));
				}
				handle_url(url);
			}
			if (m_consecutive_error_count > 20) break;
		}

		m_finished = true;
	}

	void scraper::handle_url(const URL &url) {
		m_buffer.resize(0);
		curl_easy_setopt(m_curl, CURLOPT_USERAGENT, user_agent().c_str());
		curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1l);
		curl_easy_setopt(m_curl, CURLOPT_MAXREDIRS, 5l);
		curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, curl_string_reader);
		curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(m_curl, CURLOPT_URL, url.str().c_str());
		curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, 30);
		curl_easy_setopt(m_curl, CURLOPT_ERRORBUFFER, m_curl_error_buffer);

		CURLcode res = curl_easy_perform(m_curl);

		if (res == CURLE_OK) {
			m_consecutive_error_count = 0;
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
				update_url(new_url, response_code, System::cur_datetime(), URL());
				if (url.canonically_different(new_url)) {
					update_url(url, 301, System::cur_datetime(), new_url); // A bit of cheeting heere, it is not sure the original url had a 301 response code.
				}
				if (response_code == 200) {
					handle_200_response(m_buffer, response_code, ip, new_url);
				} else {
					handle_non_200_response(m_buffer, response_code, ip, new_url);
				}
			} else {
				update_url(url, response_code, System::cur_datetime(), URL());
				if (response_code == 200) {
					handle_200_response(m_buffer, response_code, ip, url);
				} else {
					handle_non_200_response(m_buffer, response_code, ip, url);
				}
			}
		} else {
			/*
			 * Handle everything here: https://curl.se/libcurl/c/libcurl-errors.html
			 * */
			vector<CURLcode> domain_errors = {
				CURLE_COULDNT_RESOLVE_HOST,
				CURLE_COULDNT_CONNECT,
			};

			handle_curl_error(url, res, string(m_curl_error_buffer));

			if (res == CURLE_COULDNT_RESOLVE_HOST || res == CURLE_COULDNT_CONNECT) {
				update_url(url, 10000 + res, System::cur_datetime(), URL());
				mark_all_urls_with_error(10000 + res);
			} else {
				update_url(url, 10000 + res, System::cur_datetime(), URL());
			}
		}

		m_buffer.resize(0);
		m_buffer.shrink_to_fit();
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

	void scraper::handle_curl_error(const URL &url, size_t curl_error, const std::string &error_msg) {
		m_num_errors++;
		m_consecutive_error_count++;
		m_store->add_curl_error(url.str() + "\t" + to_string(curl_error) + "\t" + error_msg + "\n");
		m_store->upload_curl_errors();
	}

	void scraper::handle_200_response(const string &data, size_t response_code, const string &ip, const URL &url) {
		m_num_200++;
		HtmlParser html_parser;
		html_parser.parse(data, url.str());

		m_num_total++;
		if (url.has_www()) m_num_www++; 
		if (url.has_https()) m_num_https++; 
		if (m_num_total == 3) upload_domain_info();

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

	void scraper::handle_non_200_response(const string &data, size_t response_code, const string &ip, const URL &url) {

		m_num_non200++;

		check_for_captcha_block(data, response_code);

		HtmlParser html_parser;
		html_parser.parse(data, url.str());

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
			m_store->add_non_200_scraper_data(line);
			m_store->upload_non_200_results();
		}
	}

	void scraper::check_for_captcha_block(const std::string &data, size_t response_code) {
		if (response_code != 200 && (data.find("Captcha") != string::npos || data.find("captcha") != string::npos)) {
			m_blocked = true;
			mark_all_urls_with_error(10000 + 999);
		}
	}

	void scraper::download_domain_data() {
		int error = UrlStore::get(m_domain, m_domain_data);
		if (error == UrlStore::ERROR) {
			LOG_INFO("Could not download domain data");
		}
	}

	void scraper::download_robots() {
		const URL robots_path = filter_url(URL("http://" + m_domain + "/robots.txt"));
		m_robots_content = simple_get(robots_path);

		scraper::upload_robots_txt(m_robots_content);
	}

	bool scraper::robots_allow_url(const URL &url) const {
		googlebot::RobotsMatcher matcher;
		bool allowed = matcher.OneAgentAllowedByRobots(m_robots_content, user_agent_token(), url.str());
		return allowed;
	}

	string scraper::simple_get(const URL &url) {
		curl_easy_setopt(m_curl, CURLOPT_USERAGENT, user_agent().c_str());
		curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1l);
		curl_easy_setopt(m_curl, CURLOPT_MAXREDIRS, 5l);
		curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, curl_string_reader);
		curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(m_curl, CURLOPT_URL, url.str().c_str());
		curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, 30);
		curl_easy_setopt(m_curl, CURLOPT_ERRORBUFFER, m_curl_error_buffer);

		m_buffer.resize(0);
		CURLcode res = curl_easy_perform(m_curl);
		if (res == CURLE_OK) {
			long response_code;
			char *new_url_str = nullptr;
			curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response_code);
			curl_easy_getinfo(m_curl, CURLINFO_EFFECTIVE_URL, &new_url_str);

			check_for_captcha_block(m_buffer, response_code);
		} else {
			/*
			 * Handle everything here: https://curl.se/libcurl/c/libcurl-errors.html
			 * */
			vector<CURLcode> domain_errors = {
				CURLE_COULDNT_RESOLVE_HOST,
				CURLE_COULDNT_CONNECT,
			};

			handle_curl_error(url, res, string(m_curl_error_buffer));

			if (res == CURLE_COULDNT_RESOLVE_HOST || res == CURLE_COULDNT_CONNECT) {
				mark_all_urls_with_error(10000 + res);
			} else {
			}
		}

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

	void scraper::upload_robots_txt(const string &robots_content) {
		UrlStore::RobotsData data;
		data.m_domain = m_domain;
		data.m_robots = robots_content;

		UrlStore::set(data);
	}

	URL scraper::filter_url(const URL &url) {
		URL ret(url);
		if (m_domain_data.m_has_https && !url.has_https()) ret.set_scheme("https");
		if (m_domain_data.m_has_www && !url.has_www()) ret.set_www(true);

		return ret;
	}

	void scraper::start_thread() {
		m_started = true;
		m_thread = std::move(thread([this](){
			this->run();
		}));
	}

	size_t curl_string_reader(char *ptr, size_t size, size_t nmemb, void *userdata) {
		const size_t byte_size = size * nmemb;
		scraper *s = static_cast<scraper *>(userdata);
		if (s->m_buffer_len < s->m_buffer.size() + byte_size) return 0;
		s->m_buffer.append(ptr, byte_size);
		return byte_size;
	}

	size_t read_max_scrapers() {
		ifstream infile("/tmp/num_scrapers");
		if (!infile.is_open()) return 0;
		size_t max_scrapers;
		infile >> max_scrapers;
		return max_scrapers;
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

		vector<string> raw_urls;
		boost::algorithm::split(raw_urls, content, boost::is_any_of("\n"));

		vector<string> urls;
		for (const string &url : raw_urls) {
			if (Text::trim(url).size()) {
				urls.push_back(url);
			}
		}

		return urls;
	}

	void run_scraper_on_urls(const vector<string> &input_urls) {
		size_t max_scrapers = 1000;
		Scraper::store store;
		Scraper::stats stats;
		map<string, unique_ptr<scraper>> scrapers;

		stats.start_thread(60); // Report statistics every minute.

		vector<string> urls = input_urls;
		while (urls.size() || scrapers.size()) {

			LOG_INFO("Starting scrapers with: " + to_string(urls.size()) + " urls");

			size_t new_max_scrapers = read_max_scrapers();
			if (new_max_scrapers) {
				max_scrapers = new_max_scrapers;
			}

			vector<string> unhandled_urls;

			for (const string &url_str : urls) {
				URL url(url_str);

				if (scrapers.count(url.host()) == 0) {
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
				if (!iter.second->started()) {
					iter.second->start_thread();
				}
			}
			
			// Wait for at least 50% of the scrapers to finish.
			while (scrapers.size() > max_scrapers * 0.8) {
				stats.start_count(urls.size());
				for (auto iter = scrapers.begin(); iter != scrapers.end(); ) {
					if (iter->second->finished()) {
						stats.count_finished(*(iter->second));
						iter = scrapers.erase(iter);
					} else {
						stats.count_unfinished(*(iter->second));
						iter++;
					}
				}
				this_thread::sleep_for(1000ms);
			}
			urls = unhandled_urls;

			// Check for new urls and append them.
			vector<string> new_urls = download_scraper_urls();
			urls.insert(urls.end(), new_urls.begin(), new_urls.end());

			if (urls.size() == 0) {
				// We don't have any new urls. Just sleep a bit before checking again.
				std::this_thread::sleep_for(std::chrono::seconds(60));
			}
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
