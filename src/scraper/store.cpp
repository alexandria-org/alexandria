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

#include "store.h"
#include "system/System.h"
#include "system/datetime.h"
#include "parser/Warc.h"

using namespace std;

namespace Scraper {

	store::store() {

	}
	
	store::~store() {
		m_upload_limit = 0;
		upload_results();
		upload_non_200_results();
		UrlStore::update_many(m_url_datas, UrlStore::update_url | UrlStore::update_redirect | UrlStore::update_http_code |
				UrlStore::update_last_visited);
	}

	void store::add_url_data(const UrlStore::UrlData &data) {
		m_lock.lock();
		m_url_datas.push_back(data);
		m_lock.unlock();
		upload_url_datas();
	}

	void store::add_domain_data(const UrlStore::DomainData &data) {
		m_lock.lock();
		m_domain_datas.push_back(data);
		m_lock.unlock();
		upload_domain_datas();
	}

	void store::add_robots_data(const UrlStore::RobotsData &data) {
		m_lock.lock();
		m_robots_datas.push_back(data);
		m_lock.unlock();
		upload_robots_datas();
	}

	void store::add_scraper_data(const std::string &line) {
		m_lock.lock();
		m_results.push_back(line);
		m_lock.unlock();
	}

	void store::add_non_200_scraper_data(const std::string &line) {
		m_lock.lock();
		m_non_200_results.push_back(line);
		m_lock.unlock();
	}

	void store::add_link_data(const std::string &links) {
		m_lock.lock();
		m_link_results.push_back(links);
		m_lock.unlock();
	}

	void store::add_curl_error(const string &line) {
		m_lock.lock();
		m_curl_errors.push_back(line);
		m_lock.unlock();
	}

	void store::upload_url_datas() {
		m_lock.lock();
		if (m_url_datas.size() > 1000) {
			vector<UrlStore::UrlData> tmp_datas;
			tmp_datas.swap(m_url_datas);
			m_lock.unlock();
			UrlStore::update_many(tmp_datas, UrlStore::update_url | UrlStore::update_redirect | UrlStore::update_http_code |
					UrlStore::update_last_visited);
			return;
		}
		m_lock.unlock();
	}

	void store::upload_domain_datas() {
		m_lock.lock();
		if (m_domain_datas.size() > 1000) {
			vector<UrlStore::DomainData> tmp_datas;
			tmp_datas.swap(m_domain_datas);
			m_lock.unlock();
			UrlStore::update_many(tmp_datas, UrlStore::update_has_https | UrlStore::update_has_www);
			return;
		}
		m_lock.unlock();
	}

	void store::upload_robots_datas() {
		m_lock.lock();
		if (m_robots_datas.size() > 1000) {
			vector<UrlStore::RobotsData> tmp_datas;
			tmp_datas.swap(m_robots_datas);
			m_lock.unlock();
			UrlStore::update_many(tmp_datas, UrlStore::update_robots);
			return;
		}
		m_lock.unlock();
	}

	void store::upload_results() {
		m_lock.lock();
		if (m_results.size() >= m_upload_limit) {
			const string all_results = boost::algorithm::join(m_results, "");
			const string all_link_results = boost::algorithm::join(m_link_results, "");

			m_results.resize(0);
			m_link_results.resize(0);

			m_lock.unlock();

			internal_upload_results(all_results, all_link_results);

			return;
		}
		m_lock.unlock();
	}

	void store::upload_non_200_results() {
		m_lock.lock();
		if (m_non_200_results.size() >= m_non_200_upload_limit) {
			const string all_results = boost::algorithm::join(m_non_200_results, "");

			m_non_200_results.resize(0);

			m_lock.unlock();

			internal_upload_non_200_results(all_results);

			return;
		}
		m_lock.unlock();
	}

	void store::upload_curl_errors() {
		m_lock.lock();
		if (m_curl_errors.size() >= m_curl_errors_upload_limit) {
			const string all_results = boost::algorithm::join(m_curl_errors, "");

			m_curl_errors.resize(0);

			m_lock.unlock();

			internal_upload_curl_errors(all_results);

			return;
		}
		m_lock.unlock();
	}

	std::string store::tail() const {
		if (m_results.size() == 0) return "";
		return m_results.back();
	}

	void store::try_upload_until_complete(const string &path, const string &data) {

		size_t retry_num = 1;
		while (Transfer::upload_gz_file(path, data) == Transfer::ERROR) {
			LOG_INFO("Error uploading file " + path + " retry no " + to_string(retry_num++));
			std::this_thread::sleep_for(std::chrono::seconds(30));
		}
	}

	void store::internal_upload_results(const string &all_results, const string &all_link_results) {
		const string thread_hash = to_string(System::thread_id());
		const string warc_path = "crawl-data/ALEXANDRIA-SCRAPER-01/files/" + thread_hash + "-" + to_string(System::cur_datetime()) + "-" +
			to_string(m_file_index++) + ".warc.gz";
		try_upload_until_complete(Warc::get_result_path(warc_path), all_results);
		try_upload_until_complete(Warc::get_link_result_path(warc_path), all_link_results);
	}

	void store::internal_upload_non_200_results(const string &all_results) {
		const string thread_hash = to_string(System::thread_id());
		const string warc_path = "crawl-data/ALEXANDRIA-SCRAPER-01/non-200-responses/" + thread_hash + "-" + to_string(System::cur_datetime()) +
			"-" + to_string(m_file_index++) + ".warc.gz";
		try_upload_until_complete(Warc::get_result_path(warc_path), all_results);
	}

	void store::internal_upload_curl_errors(const string &all_results) {
		const string thread_hash = to_string(System::thread_id());
		const string warc_path = "crawl-data/ALEXANDRIA-SCRAPER-01/curl-errors/" + thread_hash + "-" + to_string(System::cur_datetime()) +
			"-" + to_string(m_file_index++) + ".warc.gz";
		try_upload_until_complete(Warc::get_result_path(warc_path), all_results);
	}

}
