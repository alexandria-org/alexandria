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
#include "parser/Warc.h"

using namespace std;

namespace Scraper {

	store::store() {

	}
	
	store::~store() {
	}

	void store::add_url_data(const UrlStore::UrlData &data) {
		m_lock.lock();
		m_url_datas.push_back(data);
		m_lock.unlock();
	}

	void store::add_scraper_data(const std::string &line) {
		m_lock.lock();
		m_results.push_back(line);
		m_lock.unlock();
	}

	void store::add_link_data(const std::string &links) {
		m_lock.lock();
		m_link_results.push_back(links);
		m_lock.unlock();
	}

	void store::upload_url_datas() {
		if (m_url_datas.size() > 1000) {
			//UrlStore::set_many()
		}
	}

	void store::upload_results() {
		m_lock.lock();
		if (m_results.size() >= m_results_limit) {
			const string all_results = boost::algorithm::join(m_results, "\n");
			const string all_link_results = boost::algorithm::join(m_link_results, "\n");

			m_results.resize(0);
			m_link_results.resize(0);

			m_lock.unlock();

			internal_upload_results(all_results, all_link_results);

			return;
		}
		m_lock.unlock();
	}

	void store::internal_upload_results(const string &all_results, const string &all_link_results) {
		const string thread_hash = to_string(System::thread_id());
		const string warc_path = "crawl-data/ALEXANDRIA-SCRAPER-01/files/" + thread_hash + "-" + to_string(m_file_index++) + ".warc.gz";
		Transfer::upload_gz_file(Warc::get_result_path(warc_path), all_results);
		Transfer::upload_gz_file(Warc::get_link_result_path(warc_path), all_link_results);
	}
}
