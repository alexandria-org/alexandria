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
#include "json.hpp"

using namespace std::literals::chrono_literals;

using json = nlohmann::json;

BOOST_AUTO_TEST_SUITE(test_url_store)

BOOST_AUTO_TEST_CASE(test_url_data) {
	URL url("https://www.example.com");
	URL url2("https://www.example2.com");
	url_store::url_data url_data;
	url_data.m_url = url;
	url_data.m_redirect = url2;
	url_data.m_http_code = 200;
	url_data.m_last_visited = 20220101;

	string string_data = url_data.to_str();

	url_store::url_data url_data2(string_data);

	BOOST_CHECK_EQUAL(url_data2.m_url.str(), "https://www.example.com");
	BOOST_CHECK_EQUAL(url_data2.m_redirect.str(), "https://www.example2.com");
	BOOST_CHECK_EQUAL(url_data2.m_http_code, 200);
	BOOST_CHECK_EQUAL(url_data2.m_last_visited, 20220101);
}

BOOST_AUTO_TEST_CASE(domain_data) {
	url_store::domain_data data;
	data.m_domain = "test.com";
	data.m_has_www = 1;
	data.m_has_https = 1;

	string string_data = data.to_str();

	url_store::domain_data data2(string_data);

	BOOST_CHECK_EQUAL(data2.m_domain, "test.com");
	BOOST_CHECK_EQUAL(data2.m_has_www, 1);
	BOOST_CHECK_EQUAL(data2.m_has_https, 1);
}

BOOST_AUTO_TEST_CASE(robots_data) {
	url_store::robots_data data;
	const string robots_content = "Sitemap: https://newsroom.cisco.com/sitemap.xml\n"
		"User-agent: NetShelter ContentScan\n"
		"Disallow: /\n"
		"User-agent: NetShelter\n"
		"Disallow: /\n"
		"User-agent: ^NetShelter\n"
		"Disallow: /\n"
		"User-agent: Mozilla/5.0 (Windows NT 6.3; WOW64; rv:36.0) Gecko/20100101 Firefox/36.0 (NetShelter ContentScan, ...)\n"
		"Disallow: /\n";
	data.m_domain = "test.com";
	data.m_robots = robots_content;

	string string_data = data.to_str();

	url_store::robots_data data2(string_data);

	BOOST_CHECK_EQUAL(data2.m_domain, "test.com");
	BOOST_CHECK_EQUAL(data2.m_robots, robots_content);
}

BOOST_AUTO_TEST_CASE(server) {

	URL url("https://www.example.com");
	URL url2 = url;
	url_store::url_data url_data;
	url_data.m_url = url;
	url_data.m_http_code = 200;
	url_data.m_last_visited = 20220101;

	url_store::set(url_data);

	url_store::url_data ret_data;
	int error = url_store::get(url.str(), ret_data);

	BOOST_CHECK_EQUAL(error, url_store::OK);
	BOOST_CHECK_EQUAL(ret_data.m_url.str(), url.str());
	BOOST_CHECK_EQUAL(ret_data.m_link_count, 0);
	BOOST_CHECK_EQUAL(ret_data.m_http_code, 200);
	BOOST_CHECK_EQUAL(ret_data.m_last_visited, 20220101);
}

BOOST_AUTO_TEST_CASE(update) {

	URL url("https://www.example.com");
	URL url2 = url;
	url_store::url_data url_data;
	url_data.m_url = url;
	url_data.m_http_code = 200;
	url_data.m_last_visited = 20220101;

	url_store::set(url_data);

	url_store::url_data url_data2;
	url_data2.m_url = url;
	url_data2.m_redirect = URL("https://www.test.com");
	url_data2.m_link_count = 10;
	url_data2.m_http_code = 0;
	url_data2.m_last_visited = 20220110;

	url_store::update(url_data2, url_store::update_last_visited | url_store::update_redirect);

	url_store::url_data ret_data;
	int error = url_store::get(url.str(), ret_data);

	BOOST_CHECK_EQUAL(error, url_store::OK);
	BOOST_CHECK_EQUAL(ret_data.m_url.str(), url.str());
	BOOST_CHECK_EQUAL(ret_data.m_redirect.str(), "https://www.test.com");
	BOOST_CHECK_EQUAL(ret_data.m_link_count, 0);
	BOOST_CHECK_EQUAL(ret_data.m_http_code, 200);
	BOOST_CHECK_EQUAL(ret_data.m_last_visited, 20220110);
}

BOOST_AUTO_TEST_CASE(get_many) {

	vector<string> urls = {
		"https://www.example1.com",
		"https://www.example2.com",
		"https://www.example3.com",
		"https://www.example4.com",
		"https://www.example5.com",
		"https://www.example6.com",
		"https://www.example7.com"
	};

	vector<url_store::url_data> datas;
	size_t idx = 1;
	for (const string &url : urls) {
		url_store::url_data url_data;
		url_data.m_url = URL(url);
		url_data.m_link_count = idx++;
		url_data.m_http_code = 200;
		url_data.m_last_visited = 20220101;
		datas.push_back(url_data);
		
	}
	url_store::set_many(datas);
	std::this_thread::sleep_for(200ms);

	vector<url_store::url_data> ret_data;
	int error = url_store::get_many(urls, ret_data);

	BOOST_CHECK_EQUAL(error, url_store::OK);
	BOOST_CHECK_EQUAL(ret_data.size(), 7);
	size_t i = 0;
	for (const string &url : urls) {
		BOOST_CHECK_EQUAL(ret_data[i].m_url.str(), url);
		BOOST_CHECK_EQUAL(ret_data[i].m_link_count, i + 1);
		BOOST_CHECK_EQUAL(ret_data[i].m_last_visited, 20220101);
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

	vector<url_store::url_data> datas;
	size_t idx = 1;
	for (const URL &url : urls) {
		url_store::url_data url_data;
		url_data.m_url = url;
		url_data.m_link_count = idx++;
		url_data.m_http_code = 200;
		url_data.m_last_visited = 20220101;
	}
	url_store::set_many(datas);
	std::this_thread::sleep_for(200ms);

	{
		transfer::Response res = transfer::get(config::url_store_host + "/store/url/https://www.example1.com");
		json json_obj = json::parse(res.body);

		BOOST_CHECK_EQUAL(json_obj["url"], "https://www.example1.com");
		BOOST_CHECK_EQUAL(json_obj["last_visited"], 20220101);
	}

	{
		vector<string> lines;
		for (const auto &url : urls) {
			lines.push_back(url.str());
		}
		transfer::Response res = transfer::post(config::url_store_host + "/store/url", boost::algorithm::join(lines, "\n"));

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

BOOST_AUTO_TEST_CASE(domain) {

	url_store::domain_data data;
	data.m_domain = "example.com";
	data.m_has_https = 1;
	data.m_has_www = 1;

	url_store::set(data);

	url_store::domain_data ret_data;
	int error = url_store::get("example.com", ret_data);

	BOOST_CHECK_EQUAL(error, url_store::OK);
	BOOST_CHECK_EQUAL(ret_data.m_domain, "example.com");
	BOOST_CHECK_EQUAL(ret_data.m_has_https, 1);
	BOOST_CHECK_EQUAL(ret_data.m_has_www, 1);

}

BOOST_AUTO_TEST_SUITE_END()
