
#include "parser/URL.h"

BOOST_AUTO_TEST_SUITE(url)

BOOST_AUTO_TEST_CASE(url_parsing) {

	URL url("https://www.facebook.com/test.html?key=value");
	BOOST_CHECK_EQUAL(url.str(), "https://www.facebook.com/test.html?key=value");
	BOOST_CHECK_EQUAL(url.domain_without_tld(), "facebook");
	BOOST_CHECK_EQUAL(url.host(), "facebook.com");
	BOOST_CHECK_EQUAL(url.host_reverse(), "com.facebook");
	BOOST_CHECK_EQUAL(url.scheme(), "https");
	BOOST_CHECK_EQUAL(url.path(), "/test.html");
	BOOST_CHECK_EQUAL(url.path_with_query(), "/test.html?key=value");
	BOOST_CHECK_EQUAL(url.size(), strlen("https://www.facebook.com/test.html?key=value"));

	auto query = url.query();
	BOOST_CHECK_EQUAL(query.size(), 1);
	BOOST_CHECK_EQUAL(query["key"], "value");
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

BOOST_AUTO_TEST_SUITE_END();
