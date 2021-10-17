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

#include "hash_table/HashTableHelper.h"
#include "api/Api.h"

BOOST_AUTO_TEST_SUITE(api)

BOOST_AUTO_TEST_CASE(api_search) {

	FullText::truncate_url_to_domain("main_index");
	FullText::truncate_index("test_main_index", 8);
	FullText::truncate_index("test_link_index", 8);

	HashTableHelper::truncate("test_main_index");
	HashTableHelper::truncate("test_link_index");
	HashTableHelper::truncate("test_domain_link_index");

	// Index full text
	{
		SubSystem *sub_system = new SubSystem();
		FullText::index_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-01", sub_system);
	}

	{
		// Index links
		UrlToDomain *url_to_domain = new UrlToDomain("main_index");
		url_to_domain->read();

		BOOST_CHECK_EQUAL(url_to_domain->size(), 8);

		SubSystem *sub_system = new SubSystem();

		FullText::index_link_batch("test_link_index", "test_domain_link_index", "test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-01",
			sub_system, url_to_domain);
	}

	HashTable hash_table("test_main_index");
	HashTable link_hash_table("test_link_index");
	vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("test_main_index", 8);
	vector<FullTextIndex<LinkFullTextRecord> *> link_index_array = FullText::create_index_array<LinkFullTextRecord>("test_link_index", 8);

	{
		stringstream response_stream;
		Api::search("url1.com", hash_table, index_array, link_index_array, {}, response_stream);

		string response = response_stream.str();

		cout << response << endl;

		Aws::Utils::Json::JsonValue json(response);

		auto v = json.View();

		BOOST_CHECK(v.ValueExists("status"));
		BOOST_CHECK_EQUAL(v.GetString("status"), "success");
		BOOST_CHECK_EQUAL(v.GetInteger("total_found"), 1);
		BOOST_CHECK_EQUAL(v.GetInteger("total_url_links_found"), 1);

		BOOST_CHECK(v.ValueExists("results"));
		BOOST_CHECK(v.GetArray("results")[0].ValueExists("url"));
		BOOST_CHECK_EQUAL(v.GetArray("results")[0].GetString("url"), "http://url1.com/test");
	}

	{
		stringstream response_stream;
		Api::word_stats("Meta Description Text", index_array, link_index_array, hash_table.size(), link_hash_table.size(), response_stream);

		string response = response_stream.str();

		Aws::Utils::Json::JsonValue json(response);

		auto v = json.View();

		BOOST_CHECK(v.ValueExists("status"));
		BOOST_CHECK_EQUAL(v.GetString("status"), "success");

		BOOST_CHECK(v.ValueExists("time_ms"));

		BOOST_CHECK(v.ValueExists("index"));
		BOOST_CHECK(v.GetObject("index").ValueExists("words"));
		BOOST_CHECK_EQUAL(v.GetObject("index").GetObject("words").GetDouble("meta"), 1.0);

		BOOST_CHECK(v.GetObject("index").ValueExists("total"));
		BOOST_CHECK_EQUAL(v.GetObject("index").GetInt64("total"), 8);
	}

	{
		stringstream response_stream;
		Api::word_stats("more uniq", index_array, link_index_array, hash_table.size(), link_hash_table.size(), response_stream);

		string response = response_stream.str();

		Aws::Utils::Json::JsonValue json(response);

		auto v = json.View();

		BOOST_CHECK(v.ValueExists("status"));
		BOOST_CHECK_EQUAL(v.GetString("status"), "success");
		BOOST_CHECK(v.ValueExists("time_ms"));

		BOOST_CHECK(v.ValueExists("index"));
		BOOST_CHECK(v.GetObject("index").ValueExists("words"));
		BOOST_CHECK_EQUAL(v.GetObject("index").GetObject("words").GetDouble("uniq"), 1.0/8.0);

		BOOST_CHECK(v.GetObject("index").ValueExists("total"));
		BOOST_CHECK_EQUAL(v.GetObject("index").GetInt64("total"), 8);

#ifdef COMPILE_WITH_LINK_INDEX
		BOOST_CHECK(v.ValueExists("link_index"));
		BOOST_CHECK(v.GetObject("link_index").ValueExists("words"));
		BOOST_CHECK_EQUAL(v.GetObject("link_index").GetObject("words").GetDouble("more"), 2.0/11.0);
		BOOST_CHECK_EQUAL(v.GetObject("link_index").GetObject("words").GetDouble("uniq"), 1.0/11.0);

		BOOST_CHECK(v.GetObject("link_index").ValueExists("total"));
		BOOST_CHECK_EQUAL(v.GetObject("link_index").GetInt64("total"), 11);
#endif
	}

	FullText::delete_index_array<FullTextRecord>(index_array);
	FullText::delete_index_array<LinkFullTextRecord>(link_index_array);
}

BOOST_AUTO_TEST_CASE(api_search_compact) {

	FullText::truncate_url_to_domain("main_index");
	FullText::truncate_index("test_main_index", 8);
	FullText::truncate_index("test_link_index", 8);

	HashTableHelper::truncate("test_main_index");
	HashTableHelper::truncate("test_link_index");
	HashTableHelper::truncate("test_domain_link_index");

	SubSystem *sub_system = new SubSystem();
	FullText::index_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-01", sub_system);

	{
		// Index links
		UrlToDomain *url_to_domain = new UrlToDomain("main_index");
		url_to_domain->read();

		BOOST_CHECK_EQUAL(url_to_domain->size(), 8);

		SubSystem *sub_system = new SubSystem();

		FullText::index_link_batch("test_link_index", "test_domain_link_index", "test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-01",
			sub_system, url_to_domain);
	}

	HashTable hash_table("test_main_index");
	HashTable link_hash_table("test_link_index");
	vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("test_main_index", 8);
	vector<FullTextIndex<LinkFullTextRecord> *> link_index_array = FullText::create_index_array<LinkFullTextRecord>("test_link_index", 8);

	{
		stringstream response_stream;
		Api::search("url1.com", hash_table, index_array, link_index_array, {}, response_stream);

		string response = response_stream.str();

		Aws::Utils::Json::JsonValue json(response);

		auto v = json.View();

		BOOST_CHECK(v.ValueExists("status"));
		BOOST_CHECK_EQUAL(v.GetString("status"), "success");

		BOOST_CHECK(v.ValueExists("results"));
		BOOST_CHECK(v.GetArray("results")[0].ValueExists("url"));
		BOOST_CHECK_EQUAL(v.GetArray("results")[0].GetString("url"), "http://url1.com/test");
	}

	{

		cout << "hash table size: " << hash_table.size() << endl;
		stringstream response_stream;
		Api::word_stats("Meta Description Text", index_array, link_index_array, hash_table.size(), link_hash_table.size(), response_stream);

		string response = response_stream.str();

		cout << response << endl;

		Aws::Utils::Json::JsonValue json(response);

		auto v = json.View();

		BOOST_CHECK(v.ValueExists("status"));
		BOOST_CHECK_EQUAL(v.GetString("status"), "success");

		BOOST_CHECK(v.ValueExists("time_ms"));

		BOOST_CHECK(v.ValueExists("index"));
		BOOST_CHECK(v.GetObject("index").ValueExists("words"));
		BOOST_CHECK_EQUAL(v.GetObject("index").GetObject("words").GetDouble("meta"), 1.0);

		BOOST_CHECK(v.GetObject("index").ValueExists("total"));
		BOOST_CHECK_EQUAL(v.GetObject("index").GetInt64("total"), 8);

#ifdef COMPILE_WITH_LINK_INDEX
		BOOST_CHECK(v.ValueExists("link_index"));
		BOOST_CHECK(v.GetObject("link_index").ValueExists("words"));
#endif
	}

	{
		stringstream response_stream;
		Api::word_stats("more uniq", index_array, link_index_array, hash_table.size(), link_hash_table.size(), response_stream);

		string response = response_stream.str();

		Aws::Utils::Json::JsonValue json(response);

		auto v = json.View();

		BOOST_CHECK(v.ValueExists("status"));
		BOOST_CHECK_EQUAL(v.GetString("status"), "success");
		BOOST_CHECK(v.ValueExists("time_ms"));

		BOOST_CHECK(v.ValueExists("index"));
		BOOST_CHECK(v.GetObject("index").ValueExists("words"));
		BOOST_CHECK_EQUAL(v.GetObject("index").GetObject("words").GetDouble("uniq"), 1.0/8.0);

		BOOST_CHECK(v.GetObject("index").ValueExists("total"));
		BOOST_CHECK_EQUAL(v.GetObject("index").GetInt64("total"), 8);

#ifdef COMPILE_WITH_LINK_INDEX
		BOOST_CHECK(v.ValueExists("link_index"));
		BOOST_CHECK(v.GetObject("link_index").ValueExists("words"));
		BOOST_CHECK_EQUAL(v.GetObject("link_index").GetObject("words").GetDouble("more"), 2.0/11.0);
		BOOST_CHECK_EQUAL(v.GetObject("link_index").GetObject("words").GetDouble("uniq"), 1.0/11.0);

		BOOST_CHECK(v.GetObject("link_index").ValueExists("total"));
		BOOST_CHECK_EQUAL(v.GetObject("link_index").GetInt64("total"), 11);
#endif
	}

	FullText::delete_index_array<FullTextRecord>(index_array);
	FullText::delete_index_array<LinkFullTextRecord>(link_index_array);
}

BOOST_AUTO_TEST_CASE(api_search_with_domain_links) {

	FullText::truncate_url_to_domain("main_index");
	FullText::truncate_index("test_main_index", 8);
	FullText::truncate_index("test_link_index", 8);
	FullText::truncate_index("test_domain_link_index", 8);

	HashTableHelper::truncate("test_main_index");
	HashTableHelper::truncate("test_link_index");
	HashTableHelper::truncate("test_domain_link_index");

	{
		// Index full text
		SubSystem *sub_system = new SubSystem();
		FullText::index_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-01", sub_system);
	}

	{
		// Index links
		UrlToDomain *url_to_domain = new UrlToDomain("main_index");
		url_to_domain->read();

		BOOST_CHECK_EQUAL(url_to_domain->size(), 8);

		SubSystem *sub_system = new SubSystem();

		FullText::index_link_batch("test_link_index", "test_domain_link_index", "test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-01",
			sub_system, url_to_domain);
	}

	HashTable hash_table("test_main_index");
	HashTable link_hash_table("test_link_index");
	vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("test_main_index", 8);
	vector<FullTextIndex<LinkFullTextRecord> *> link_index_array = FullText::create_index_array<LinkFullTextRecord>("test_link_index", 8);
	vector<FullTextIndex<DomainLinkFullTextRecord> *> domain_link_index_array =
		FullText::create_index_array<DomainLinkFullTextRecord>("test_domain_link_index", 8);

	{
		stringstream response_stream;
		Api::search("url1.com", hash_table, index_array, link_index_array, domain_link_index_array, response_stream);

		string response = response_stream.str();

		Aws::Utils::Json::JsonValue json(response);

		auto v = json.View();

		BOOST_CHECK(v.ValueExists("status"));
		BOOST_CHECK_EQUAL(v.GetString("status"), "success");
		BOOST_CHECK_EQUAL(v.GetInteger("total_found"), 1);
		BOOST_CHECK_EQUAL(v.GetInteger("total_domain_links_found"), 2);

		BOOST_CHECK(v.ValueExists("results"));
		BOOST_CHECK(v.GetArray("results")[0].ValueExists("url"));
		BOOST_CHECK_EQUAL(v.GetArray("results")[0].GetString("url"), "http://url1.com/test");
	}

	FullText::delete_index_array<FullTextRecord>(index_array);
	FullText::delete_index_array<LinkFullTextRecord>(link_index_array);
	FullText::delete_index_array<DomainLinkFullTextRecord>(domain_link_index_array);
}

BOOST_AUTO_TEST_CASE(many_links) {

	FullText::truncate_url_to_domain("main_index");
	FullText::truncate_index("test_main_index", 8);
	FullText::truncate_index("test_link_index", 8);

	HashTableHelper::truncate("test_main_index");
	HashTableHelper::truncate("test_link_index");
	HashTableHelper::truncate("test_domain_link_index");

	{
		// Index full text
		SubSystem *sub_system = new SubSystem();
		FullText::index_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-06", sub_system);
	}

	{
		// Index links
		UrlToDomain *url_to_domain = new UrlToDomain("main_index");
		url_to_domain->read();

		SubSystem *sub_system = new SubSystem();

		FullText::index_link_batch("test_link_index", "test_domain_link_index", "test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-06",
			sub_system, url_to_domain);
	}

	HashTable hash_table("test_main_index");
	vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("test_main_index", 8);
	vector<FullTextIndex<LinkFullTextRecord> *> link_index_array = FullText::create_index_array<LinkFullTextRecord>("test_link_index", 8);
	vector<FullTextIndex<DomainLinkFullTextRecord> *> domain_link_index_array =
		FullText::create_index_array<DomainLinkFullTextRecord>("test_domain_link_index", 8);

	{
		stringstream response_stream;
		Api::search("url1.com", hash_table, index_array, link_index_array, domain_link_index_array, response_stream);

		string response = response_stream.str();

		Aws::Utils::Json::JsonValue json(response);

		auto v = json.View();

		BOOST_CHECK(v.ValueExists("status"));
		BOOST_CHECK_EQUAL(v.GetString("status"), "success");
		BOOST_CHECK_EQUAL(v.GetInteger("total_found"), 6);
		BOOST_CHECK_EQUAL(v.GetInteger("link_url_matches"), 15);
	}

	FullText::delete_index_array<FullTextRecord>(index_array);
	FullText::delete_index_array<LinkFullTextRecord>(link_index_array);
	FullText::delete_index_array<DomainLinkFullTextRecord>(domain_link_index_array);

}

BOOST_AUTO_TEST_CASE(api_word_stats) {

	FullText::truncate_url_to_domain("main_index");
	FullText::truncate_index("test_main_index", 8);
	FullText::truncate_index("test_link_index", 8);

	HashTableHelper::truncate("test_main_index");
	HashTableHelper::truncate("test_link_index");
	HashTableHelper::truncate("test_domain_link_index");

	{
		// Index full text
		SubSystem *sub_system = new SubSystem();
		FullText::index_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-02", sub_system);
	}

	{
		// Index links
		UrlToDomain *url_to_domain = new UrlToDomain("main_index");
		url_to_domain->read();

		BOOST_CHECK_EQUAL(url_to_domain->size(), 8);

		SubSystem *sub_system = new SubSystem();

		FullText::index_link_batch("test_link_index", "test_domain_link_index", "test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-02",
			sub_system, url_to_domain);
	}

	HashTable hash_table("test_main_index");
	HashTable link_hash_table("test_link_index");
	vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("test_main_index", 8);
	vector<FullTextIndex<LinkFullTextRecord> *> link_index_array = FullText::create_index_array<LinkFullTextRecord>("test_link_index", 8);

	{
		stringstream response_stream;
		Api::word_stats("uniq", index_array, link_index_array, hash_table.size(), link_hash_table.size(), response_stream);

		string response = response_stream.str();

		Aws::Utils::Json::JsonValue json(response);

		auto v = json.View();

		BOOST_CHECK(v.ValueExists("status"));
		BOOST_CHECK_EQUAL(v.GetString("status"), "success");

		BOOST_CHECK(v.ValueExists("time_ms"));

		BOOST_CHECK(v.ValueExists("index"));
		BOOST_CHECK(v.GetObject("index").ValueExists("words"));
		BOOST_CHECK_EQUAL(v.GetObject("index").GetObject("words").GetDouble("uniq"), 2.0/8.0);

		BOOST_CHECK(v.GetObject("index").ValueExists("total"));
		BOOST_CHECK_EQUAL(v.GetObject("index").GetInt64("total"), 8);

#ifdef COMPILE_WITH_LINK_INDEX
		BOOST_CHECK(v.ValueExists("link_index"));
		BOOST_CHECK(v.GetObject("link_index").ValueExists("words"));
		BOOST_CHECK_EQUAL(v.GetObject("link_index").GetObject("words").GetDouble("uniq"), 0.0);

		BOOST_CHECK(v.GetObject("link_index").ValueExists("total"));
		BOOST_CHECK_EQUAL(v.GetObject("link_index").GetInt64("total"), 4);
#endif
	}

	{
		stringstream response_stream;
		Api::word_stats("test07.links.gz", index_array, link_index_array, hash_table.size(), link_hash_table.size(), response_stream);

		string response = response_stream.str();

		Aws::Utils::Json::JsonValue json(response);

		auto v = json.View();

		BOOST_CHECK(v.ValueExists("status"));
		BOOST_CHECK_EQUAL(v.GetString("status"), "success");

		BOOST_CHECK(v.ValueExists("time_ms"));

#ifdef COMPILE_WITH_LINK_INDEX
		BOOST_CHECK(v.ValueExists("link_index"));
		BOOST_CHECK(v.GetObject("link_index").ValueExists("words"));
		BOOST_CHECK_EQUAL(v.GetObject("link_index").GetObject("words").GetDouble("test07.links.gz"), 1.0/4.0);

		BOOST_CHECK(v.GetObject("link_index").ValueExists("total"));
		BOOST_CHECK_EQUAL(v.GetObject("link_index").GetInt64("total"), 4);
#endif
	}

	FullText::delete_index_array<FullTextRecord>(index_array);
	FullText::delete_index_array<LinkFullTextRecord>(link_index_array);
}

BOOST_AUTO_TEST_CASE(api_hash_table) {

	FullText::truncate_url_to_domain("main_index");
	FullText::truncate_index("test_main_index", 8);
	FullText::truncate_index("test_link_index", 8);

	HashTableHelper::truncate("test_main_index");
	HashTableHelper::truncate("test_link_index");
	HashTableHelper::truncate("test_domain_link_index");

	{
		// Index full text
		SubSystem *sub_system = new SubSystem();
		FullText::index_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-04", sub_system);
	}

	{
		// Index links
		UrlToDomain *url_to_domain = new UrlToDomain("main_index");
		url_to_domain->read();

		SubSystem *sub_system = new SubSystem();

		FullText::index_link_batch("test_link_index", "test_domain_link_index", "test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-04",
			sub_system, url_to_domain);
	}

	HashTable hash_table("test_main_index");

	{
		stringstream response_stream;
		Api::url("http://url1.com/my_test_url", hash_table, response_stream);

		string response = response_stream.str();

		Aws::Utils::Json::JsonValue json(response);

		auto v = json.View();

		BOOST_CHECK(v.ValueExists("status"));
		BOOST_CHECK_EQUAL(v.GetString("status"), "success");

		BOOST_CHECK(v.ValueExists("time_ms"));
		BOOST_CHECK(v.ValueExists("response"));

		BOOST_CHECK_EQUAL(v.GetString("response"), "http://url1.com/my_test_url	test test		test test test test		ALEXANDRIA-TEST-04");
	}

	{
		stringstream response_stream;
		Api::url("http://non-existing-url.com", hash_table, response_stream);

		string response = response_stream.str();

		Aws::Utils::Json::JsonValue json(response);

		auto v = json.View();

		BOOST_CHECK(v.ValueExists("status"));
		BOOST_CHECK_EQUAL(v.GetString("status"), "success");

		BOOST_CHECK(v.ValueExists("time_ms"));
		BOOST_CHECK(v.ValueExists("response"));

		BOOST_CHECK_EQUAL(v.GetString("response"), "");
	}

}

BOOST_AUTO_TEST_CASE(api_search_deduplication_on_nodes) {

	FullText::truncate_url_to_domain("main_index");
	FullText::truncate_index("test_main_index", 8);
	FullText::truncate_index("test_link_index", 8);
	FullText::truncate_index("test_domain_link_index", 8);

	HashTableHelper::truncate("test_main_index");
	HashTableHelper::truncate("test_link_index");
	HashTableHelper::truncate("test_domain_link_index");

	Config::nodes_in_cluster = 2;
	Config::node_id = 0;

	URL url("http://url1.com");
	cout << url.str() << " host hash mod 16: " << (url.host_hash() % 16) << endl;

	// url8.com should be in node 0
	// url1-7.com should be in node 1

	{
		// Index full text
		SubSystem *sub_system = new SubSystem();
		FullText::index_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-01", sub_system);
	}

	{
		// Index links
		UrlToDomain *url_to_domain = new UrlToDomain("main_index");
		url_to_domain->read();

		SubSystem *sub_system = new SubSystem();

		FullText::index_link_batch("test_link_index", "test_domain_link_index", "test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-01",
			sub_system, url_to_domain);
	}

	HashTable hash_table("test_main_index");
	HashTable link_hash_table("test_link_index");
	vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("test_main_index", 8);
	vector<FullTextIndex<LinkFullTextRecord> *> link_index_array = FullText::create_index_array<LinkFullTextRecord>("test_link_index", 8);

	{
		stringstream response_stream;
		Api::search("url1.com", hash_table, index_array, link_index_array, {}, response_stream);

		string response = response_stream.str();

		Aws::Utils::Json::JsonValue json(response);

		auto v = json.View();

		BOOST_CHECK(v.ValueExists("status"));
		BOOST_CHECK_EQUAL(v.GetString("status"), "success");

		BOOST_CHECK(v.ValueExists("results"));
		BOOST_CHECK_EQUAL(v.GetArray("results").GetLength(), 0);
	}

	FullText::delete_index_array(index_array);
	FullText::delete_index_array(link_index_array);

	// Reset config.
	Config::nodes_in_cluster = 1;
	Config::node_id = 0;
}

BOOST_AUTO_TEST_CASE(api_search_deduplication) {

	FullText::truncate_url_to_domain("main_index");
	FullText::truncate_index("test_main_index", 8);
	FullText::truncate_index("test_link_index", 8);
	FullText::truncate_index("test_domain_link_index", 8);

	HashTableHelper::truncate("test_main_index");
	HashTableHelper::truncate("test_link_index");
	HashTableHelper::truncate("test_domain_link_index");

	{
		// Index full text
		SubSystem *sub_system = new SubSystem();
		FullText::index_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-06", sub_system);
	}

	{
		// Index links
		UrlToDomain *url_to_domain = new UrlToDomain("main_index");
		url_to_domain->read();

		SubSystem *sub_system = new SubSystem();

		FullText::index_link_batch("test_link_index", "test_domain_link_index", "test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-06",
			sub_system, url_to_domain);
	}

	HashTable hash_table("test_main_index");
	vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("test_main_index", 8);
	vector<FullTextIndex<LinkFullTextRecord> *> link_index_array = FullText::create_index_array<LinkFullTextRecord>("test_link_index", 8);
	vector<FullTextIndex<DomainLinkFullTextRecord> *> domain_link_index_array =
		FullText::create_index_array<DomainLinkFullTextRecord>("test_domain_link_index", 8);

	{
		stringstream response_stream;
		Api::search("url2.com", hash_table, index_array, link_index_array, {}, response_stream);

		string response = response_stream.str();

		Aws::Utils::Json::JsonValue json(response);

		auto v = json.View();

		BOOST_CHECK(v.ValueExists("status"));
		BOOST_CHECK_EQUAL(v.GetString("status"), "success");

		BOOST_CHECK(v.ValueExists("results"));
		BOOST_CHECK_EQUAL(v.GetArray("results").GetLength(), 6);
	}

	{
		stringstream response_stream;
		Api::search_all("site:url2.com", hash_table, index_array, link_index_array, domain_link_index_array, response_stream);

		string response = response_stream.str();

		Aws::Utils::Json::JsonValue json(response);

		auto v = json.View();

		BOOST_CHECK(v.ValueExists("status"));
		BOOST_CHECK_EQUAL(v.GetString("status"), "success");

		BOOST_CHECK(v.ValueExists("results"));
		BOOST_CHECK_EQUAL(v.GetArray("results").GetLength(), 19);
	}

	FullText::delete_index_array<FullTextRecord>(index_array);
	FullText::delete_index_array<LinkFullTextRecord>(link_index_array);
	FullText::delete_index_array<DomainLinkFullTextRecord>(domain_link_index_array);

}

BOOST_AUTO_TEST_SUITE_END();
