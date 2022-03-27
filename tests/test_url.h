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

#include "url.h"

BOOST_AUTO_TEST_SUITE(test_url)

BOOST_AUTO_TEST_CASE(basic) {
	BOOST_CHECK_EQUAL(URL("https://www.facebook.com/test.html?key=value").str(), "https://www.facebook.com/test.html?key=value");

	{
		URL url("https://www.facebook.com/test.html?key=value");
		url.set_scheme("http");
		url.set_www(false);
		
		BOOST_CHECK_EQUAL(url.str(), "http://facebook.com/test.html?key=value");

		url.set_scheme("https");
		url.set_www(true);

		BOOST_CHECK_EQUAL(url.str(), "https://www.facebook.com/test.html?key=value");
	}
}

BOOST_AUTO_TEST_CASE(url_parsing) {

	{
		URL url("https://www.facebook.com/test.html?key=value");
		BOOST_CHECK_EQUAL(url.str(), "https://www.facebook.com/test.html?key=value");
		BOOST_CHECK_EQUAL(url.domain_without_tld(), "facebook");
		BOOST_CHECK_EQUAL(url.host(), "facebook.com");
		BOOST_CHECK_EQUAL(url.host_reverse(), "com.facebook");
		BOOST_CHECK_EQUAL(url.scheme(), "https");
		BOOST_CHECK_EQUAL(url.path(), "/test.html");
		BOOST_CHECK_EQUAL(url.path_with_query(), "/test.html?key=value");
		BOOST_CHECK_EQUAL(url.size(), strlen("https://www.facebook.com/test.html?key=value"));
		BOOST_CHECK_EQUAL(url.has_https(), true);
		BOOST_CHECK_EQUAL(url.has_www(), true);

		auto query = url.query();
		BOOST_CHECK_EQUAL(query.size(), 1);
		BOOST_CHECK_EQUAL(query["key"], "value");
	}
	{
		URL url("http://example.com/");
		BOOST_CHECK_EQUAL(url.has_https(), false);
		BOOST_CHECK_EQUAL(url.has_www(), false);
	}
}

BOOST_AUTO_TEST_CASE(url_parsing2) {

	URL url("https://github.com/joscul/alexandria/blob/main/tests/File.h");
	BOOST_CHECK_EQUAL(url.domain_without_tld(), "github");
	BOOST_CHECK_EQUAL(url.host(), "github.com");
	BOOST_CHECK_EQUAL(url.scheme(), "https");
	BOOST_CHECK_EQUAL(url.path(), "/joscul/alexandria/blob/main/tests/File.h");
	BOOST_CHECK_EQUAL(url.path_with_query(), "/joscul/alexandria/blob/main/tests/File.h");

	auto query = url.query();
	BOOST_CHECK_EQUAL(query.size(), 0);
}

BOOST_AUTO_TEST_CASE(hash) {

	URL url("https://github.com/joscul/alexandria/blob/main/tests/File.h");

	size_t hash1 = URL("https://github.com/joscul/alexandria/blob/main/tests/File.h").hash();
	size_t hash2 = URL("https://github.com/joscul/alexandria/blob/main/tests/File.h?query=param").hash();
	size_t hash3 = URL("https://github.com/joscul/alexandria/blob/main/tests/File.h?hej=hopp").hash();
	size_t hash4 = URL("https://www.github.com/joscul/alexandria/blob/main/tests/File.h?hej=hopp").hash();
	size_t hash5 = URL("http://github.com/joscul/alexandria/blob/main/tests/File.h?hej=hopp").hash();

	BOOST_CHECK(hash1 != hash2);
	BOOST_CHECK(hash2 != hash3);
	BOOST_CHECK(hash3 == hash4);
	BOOST_CHECK(hash4 == hash5);
}

BOOST_AUTO_TEST_CASE(unescape) {

	{
		URL url("https://github.com/?q=test%20test");
		map<string, string> query = url.query();

		BOOST_CHECK_EQUAL(query["q"], "test test");
	}
	{
		URL url("https://github.com/?q=test%2020");
		map<string, string> query = url.query();

		BOOST_CHECK_EQUAL(query["q"], "test 20");
	}
	{
		URL url("https://github.com/search?q=targumical&cp=0&hl=en-US&pq=%targumical%&sourceid=chrome&ie=UTF-8");
		map<string, string> query = url.query();

		BOOST_CHECK_EQUAL(query["pq"], "%targumical%");
	}

	{
		URL url("https://github.com/search?q=stress%%c3%C3%a5%C3%A4%c3%b6%0G");
		map<string, string> query = url.query();

		BOOST_CHECK_EQUAL(query["q"], "stress%c3åäö%0G");
	}

	{
		// Test double encoding.
		URL url("https://github.com/search?q=%25C3%25A5%25C3%25A4%25C3%25B6");
		map<string, string> query = url.query();

		BOOST_CHECK_EQUAL(query["q"], "%C3%A5%C3%A4%C3%B6");
	}

	{
		// Test double encoding.
		URL url("https://github.com/search?q=%josef%0");
		map<string, string> query = url.query();

		BOOST_CHECK_EQUAL(query["q"], "%josef%0");
	}

}

BOOST_AUTO_TEST_SUITE_END()
