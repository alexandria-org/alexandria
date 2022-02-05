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

#include <iostream>
#include <queue>
#include <curl/curl.h>
#include "robots.h"
#include "store.h"
#include "parser/URL.h"
#include "urlstore/DomainData.h"
#include "system/Profiler.h"

namespace Scraper {

	std::string user_agent_token();
	std::string user_agent();

	/*
	 * The scraper!
	 * */
	class scraper {
		public:

			scraper(const std::string &domain, store *store);
			~scraper();

			void set_timeout(size_t timeout) { m_timeout = timeout; }
			void push_url(const URL &url);
			void run();
			void start_thread();
			bool finished() { return m_finished; };
			bool started() { return m_started; }
			std::string domain() { return m_domain; }
			size_t num_scraped() const { return m_num_200; }
			size_t num_scraped_non200() const { return m_num_non200; }
			bool blocked() const { return m_blocked; }

		private:
			std::thread m_thread;
			bool m_started = false;
			bool m_finished = false;
			std::string m_domain;
			std::string m_buffer;
			size_t m_buffer_len = 1024*1024*10;
			size_t m_num_200 = 0;
			size_t m_num_non200 = 0;
			bool m_blocked = false;
			CURL *m_curl;
			store *m_store;
			std::queue<URL> m_queue;
			googlebot::RobotsMatcher m_robots;
			UrlStore::DomainData m_domain_data;
			std::string m_robots_content;
			size_t m_num_total = 0;
			size_t m_num_www = 0;
			size_t m_num_https = 0;
			size_t m_consecutive_error_count = 0;
			size_t m_timeout = 100;

			void handle_error(const std::string &error);
			void handle_url(const URL &url);
			void mark_all_urls_with_error(size_t error_code);
			void update_url(const URL &url, size_t http_code, size_t last_visited, const URL &redirect);
			void handle_200_response(const std::string &data, size_t response_code, const std::string &ip, const URL &url);
			void handle_non_200_response(const std::string &data, size_t response_code, const std::string &ip, const URL &url);
			void download_domain_data();
			void download_robots();
			bool robots_allow_url(const URL &url) const;
			std::string simple_get(const URL &url);
			void upload_domain_info();
			URL filter_url(const URL &url);

		public:

			friend size_t curl_string_reader(char *ptr, size_t size, size_t nmemb, void *userdata);
	};

	class stats {
		public:
			stats();
			~stats();
			void start_thread(size_t timeout);
			void start_count();
			void count_finished(const scraper &scraper);
			void count_unfinished(const scraper &scraper);

		private:
			std::thread m_thread;
			size_t m_timeout = 300;
			size_t m_num_blocked = 0;
			size_t m_finished_scrapers = 0;
			size_t m_unfinished_scrapers = 0;
			size_t m_scraped_urls = 0;
			size_t m_unfinished_scraped_urls = 0;
			size_t m_scraped_urls_non200 = 0;
			size_t m_unfinished_scraped_urls_non200 = 0;
			bool m_running = true;

			void run();
			void log_report(size_t dt);
	};

	size_t curl_string_reader(char *ptr, size_t size, size_t nmemb, void *userdata);

	void url_downloader();
	void run_scraper_on_urls(const std::vector<std::string> &input_urls);

}
