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

BOOST_AUTO_TEST_SUITE(url_store)

BOOST_AUTO_TEST_CASE(local) {
	URL url("https://www.example.com");
	UrlStore::UrlData url_data = {
		.url = url.str(),
		.link_count = 0,
		.http_code = 200,
		.location = "",
		.last_visited = 20220101
	};
	UrlStore::UrlStore url_db("/tmp/testdb");
	url_db.set(url, url_data);
	url_data = url_db.get(url);

	BOOST_CHECK_EQUAL(url_data.url, url.str());
	BOOST_CHECK_EQUAL(url_data.link_count, 0);
	BOOST_CHECK_EQUAL(url_data.http_code, 200);
	BOOST_CHECK_EQUAL(url_data.location, "");
	BOOST_CHECK_EQUAL(url_data.last_visited, 20220101);

	UrlStore::print_url_data(url_data);
}

BOOST_AUTO_TEST_SUITE_END()
