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

#include "urlstore/UrlStore.h"
#include "urlstore/UrlData.h"

namespace Scraper {

	/*
	 * Responsible for storing scraper data on a file and upload it to our fileserver when the file reaches a number of urls.
	 * */
	class store {
		public:
			store();
			~store();

			void add_url_data(const UrlStore::UrlData &data);
			void add_scraper_data(const std::string &line);
			void add_link_data(const std::string &links);
			void upload_url_datas();
			void upload_results();

		private:
			std::mutex m_lock;
			std::vector<UrlStore::UrlData> m_url_datas;
			std::vector<std::string> m_results;
			std::vector<std::string> m_link_results;
			size_t m_file_index = 0;
			size_t m_upload_limit = 150000;

			void internal_upload_results(const std::string &all_results, const std::string &all_link_results);

	};

}
