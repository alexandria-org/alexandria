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
#include "json.hpp"

using json = nlohmann::json;

BOOST_AUTO_TEST_SUITE(url_store)

BOOST_AUTO_TEST_CASE(local) {
	URL url("https://www.example.com");
	UrlStore::UrlData url_data = {
		.url = url,
		.redirect = URL(),
		.link_count = 0,
		.http_code = 200,
		.last_visited = 20220101
	};
	UrlStore::UrlStore url_db("/tmp/testdb");
	url_db.set(url_data);
	url_data = url_db.get(url);

	BOOST_CHECK_EQUAL(url_data.url.str(), url.str());
	BOOST_CHECK_EQUAL(url_data.link_count, 0);
	BOOST_CHECK_EQUAL(url_data.http_code, 200);
	BOOST_CHECK_EQUAL(url_data.last_visited, 20220101);
}

BOOST_AUTO_TEST_CASE(server) {

	URL url("https://www.example.com");
	URL url2 = url;
	UrlStore::UrlData url_data = {
		.url = url,
		.redirect = URL(),
		.link_count = 0,
		.http_code = 200,
		.last_visited = 20220101
	};
	UrlStore::set(url_data);

	UrlStore::UrlData ret_data;
	int error = UrlStore::get(url, ret_data);

	BOOST_CHECK_EQUAL(error, UrlStore::OK);
	BOOST_CHECK_EQUAL(ret_data.url.str(), url.str());
	BOOST_CHECK_EQUAL(ret_data.link_count, 0);
	BOOST_CHECK_EQUAL(ret_data.http_code, 200);
	BOOST_CHECK_EQUAL(ret_data.last_visited, 20220101);

}

BOOST_AUTO_TEST_CASE(update) {


	URL url("https://www.example.com");
	URL url2 = url;
	UrlStore::UrlData url_data = {
		.url = url,
		.redirect = URL(),
		.link_count = 0,
		.http_code = 200,
		.last_visited = 20220101
	};
	UrlStore::set(url_data);

	UrlStore::UrlData url_data2 = {
		.url = url,
		.redirect = URL("https://www.test.com"),
		.link_count = 10,
		.http_code = 0,
		.last_visited = 20220110
	};

	UrlStore::update(url_data2, UrlStore::update_last_visited | UrlStore::update_redirect);

	UrlStore::UrlData ret_data;
	int error = UrlStore::get(url, ret_data);

	BOOST_CHECK_EQUAL(error, UrlStore::OK);
	BOOST_CHECK_EQUAL(ret_data.url.str(), url.str());
	BOOST_CHECK_EQUAL(ret_data.redirect.str(), "https://www.test.com");
	BOOST_CHECK_EQUAL(ret_data.link_count, 0);
	BOOST_CHECK_EQUAL(ret_data.http_code, 200);
	BOOST_CHECK_EQUAL(ret_data.last_visited, 20220110);
}

BOOST_AUTO_TEST_CASE(get_many) {

	vector<URL> urls = {
		URL("https://www.example1.com"),
		URL("https://www.example2.com"),
		URL("https://www.example3.com"),
		URL("https://www.example4.com"),
		URL("https://www.example5.com"),
		URL("https://www.example6.com"),
		URL("https://www.example7.com")
	};

	vector<UrlStore::UrlData> datas;
	size_t idx = 1;
	for (const URL &url : urls) {
		datas.emplace_back(UrlStore::UrlData{
			.url = url,
			.redirect = URL(),
			.link_count = idx++,
			.http_code = 200,
			.last_visited = 20220101
		});
	}
	UrlStore::set(datas);

	vector<UrlStore::UrlData> ret_data;
	int error = UrlStore::get(urls, ret_data);

	BOOST_CHECK_EQUAL(error, UrlStore::OK);
	BOOST_CHECK_EQUAL(ret_data.size(), 7);
	size_t i = 0;
	for (const URL &url : urls) {
		BOOST_CHECK_EQUAL(ret_data[i].url.str(), url.str());
		BOOST_CHECK_EQUAL(ret_data[i].link_count, i + 1);
		BOOST_CHECK_EQUAL(ret_data[i].last_visited, 20220101);
		i++;
	}
}

BOOST_AUTO_TEST_CASE(get_json) {

	vector<URL> urls = {
		URL("https://www.example1.com"),
		URL("https://www.example2.com"),
		URL("https://www.example3.com"),
		URL("https://www.example4.com"),
		URL("https://www.example5.com"),
		URL("https://www.example6.com"),
		URL("https://www.example7.com")
	};

	vector<UrlStore::UrlData> datas;
	size_t idx = 1;
	for (const URL &url : urls) {
		datas.emplace_back(UrlStore::UrlData{
			.url = url,
			.redirect = URL(),
			.link_count = idx++,
			.http_code = 200,
			.last_visited = 20220101
		});
	}
	UrlStore::set(datas);

	{
		Transfer::Response res = Transfer::get(Config::url_store_host + "/urlstore/https://www.example1.com");
		json json_obj = json::parse(res.body);

		BOOST_CHECK_EQUAL(json_obj["url"], "https://www.example1.com");
		BOOST_CHECK_EQUAL(json_obj["last_visited"], 20220101);
	}

	{
		vector<string> lines;
		for (const auto &url : urls) {
			lines.push_back(url.str());
		}
		Transfer::Response res = Transfer::post(Config::url_store_host + "/urlstore", boost::algorithm::join(lines, "\n"));

		json ret_data = json::parse(res.body);

		BOOST_CHECK_EQUAL(ret_data.size(), 7);
		size_t i = 0;
		for (const URL &url : urls) {
			BOOST_CHECK_EQUAL(ret_data[i]["url"], url.str());
			BOOST_CHECK_EQUAL(ret_data[i]["link_count"], i + 1);
			BOOST_CHECK_EQUAL(ret_data[i]["last_visited"], 20220101);
			i++;
		}
	}

}

BOOST_AUTO_TEST_SUITE_END()
