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

#include "url_store/url_store.h"
#include "url_store/url_data.h"
#include "url_store/domain_data.h"
#include "url_store/robots_data.h"

namespace scraper {

	/*
	 * Responsible for storing scraper data on a file and upload it to our fileserver when the file reaches a number of urls.
	 * */
	class scraper_store {
		public:
			scraper_store();
			~scraper_store();

			void add_url_data(const url_store::url_data &data);
			void add_domain_data(const url_store::domain_data &data);
			void add_robots_data(const url_store::robots_data &data);
			void add_scraper_data(const std::string &line);
			void add_non_200_scraper_data(const std::string &line);
			void add_link_data(const std::string &links);
			void add_curl_error(const std::string &line);
			void upload_url_datas();
			void upload_domain_datas();
			void upload_robots_datas();
			void upload_results();
			void upload_non_200_results();
			void upload_curl_errors();
			std::string tail() const;

		private:
			std::mutex m_lock;
			std::vector<url_store::url_data> m_url_datas;
			std::vector<url_store::domain_data> m_domain_datas;
			std::vector<url_store::robots_data> m_robots_datas;
			std::vector<std::string> m_results;
			std::vector<std::string> m_non_200_results;
			std::vector<std::string> m_link_results;
			std::vector<std::string> m_curl_errors;
			size_t m_file_index = 0;
			size_t m_upload_limit = 50000;
			size_t m_non_200_upload_limit = 10000;
			size_t m_curl_errors_upload_limit = 10000;

			void try_upload_until_complete(const std::string &path, const std::string &data);
			void internal_upload_results(const std::string &all_results, const std::string &all_link_results);
			void internal_upload_non_200_results(const std::string &all_results);
			void internal_upload_curl_errors(const std::string &all_results);

	};

}
