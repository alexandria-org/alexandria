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

	SearchAllocation::Allocation *allocation = SearchAllocation::create_allocation();

	FullText::truncate_url_to_domain("main_index");
	FullText::truncate_index("test_main_index");
	FullText::truncate_index("test_link_index");

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
	vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("test_main_index");
	vector<FullTextIndex<LinkFullTextRecord> *> link_index_array = FullText::create_index_array<LinkFullTextRecord>("test_link_index");

	{
		stringstream response_stream;
		Api::search("url1.com", hash_table, index_array, link_index_array, {}, allocation, response_stream);

		string response = response_stream.str();

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
	}

	FullText::delete_index_array<FullTextRecord>(index_array);
	FullText::delete_index_array<LinkFullTextRecord>(link_index_array);

	SearchAllocation::delete_allocation(allocation);
}

BOOST_AUTO_TEST_CASE(api_search_compact) {

	SearchAllocation::Allocation *allocation = SearchAllocation::create_allocation();

	FullText::truncate_url_to_domain("main_index");
	FullText::truncate_index("test_main_index");
	FullText::truncate_index("test_link_index");

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
	vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("test_main_index");
	vector<FullTextIndex<LinkFullTextRecord> *> link_index_array = FullText::create_index_array<LinkFullTextRecord>("test_link_index");

	{
		stringstream response_stream;
		Api::search("url1.com", hash_table, index_array, link_index_array, {}, allocation, response_stream);

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
	}

	FullText::delete_index_array<FullTextRecord>(index_array);
	FullText::delete_index_array<LinkFullTextRecord>(link_index_array);

	SearchAllocation::delete_allocation(allocation);
}

BOOST_AUTO_TEST_CASE(api_search_with_domain_links) {

	SearchAllocation::Allocation *allocation = SearchAllocation::create_allocation();

	FullText::truncate_url_to_domain("main_index");
	FullText::truncate_index("test_main_index");
	FullText::truncate_index("test_link_index");
	FullText::truncate_index("test_domain_link_index");

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
	vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("test_main_index");
	vector<FullTextIndex<LinkFullTextRecord> *> link_index_array = FullText::create_index_array<LinkFullTextRecord>("test_link_index");
	vector<FullTextIndex<DomainLinkFullTextRecord> *> domain_link_index_array =
		FullText::create_index_array<DomainLinkFullTextRecord>("test_domain_link_index");

	{
		stringstream response_stream;
		Api::search("url1.com", hash_table, index_array, link_index_array, domain_link_index_array, allocation, response_stream);

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

	SearchAllocation::delete_allocation(allocation);
}

BOOST_AUTO_TEST_CASE(many_links) {

	SearchAllocation::Allocation *allocation = SearchAllocation::create_allocation();

	FullText::truncate_url_to_domain("main_index");
	FullText::truncate_index("test_main_index");
	FullText::truncate_index("test_link_index");

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
	vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("test_main_index");
	vector<FullTextIndex<LinkFullTextRecord> *> link_index_array = FullText::create_index_array<LinkFullTextRecord>("test_link_index");
	vector<FullTextIndex<DomainLinkFullTextRecord> *> domain_link_index_array =
		FullText::create_index_array<DomainLinkFullTextRecord>("test_domain_link_index");

	{
		stringstream response_stream;
		Api::search("url1.com", hash_table, index_array, link_index_array, domain_link_index_array, allocation, response_stream);

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

	SearchAllocation::delete_allocation(allocation);

}

BOOST_AUTO_TEST_CASE(api_word_stats) {

	FullText::truncate_url_to_domain("main_index");
	FullText::truncate_index("test_main_index");
	FullText::truncate_index("test_link_index");

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
	vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("test_main_index");
	vector<FullTextIndex<LinkFullTextRecord> *> link_index_array = FullText::create_index_array<LinkFullTextRecord>("test_link_index");

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
	}

	FullText::delete_index_array<FullTextRecord>(index_array);
	FullText::delete_index_array<LinkFullTextRecord>(link_index_array);
}

BOOST_AUTO_TEST_CASE(api_hash_table) {

	FullText::truncate_url_to_domain("main_index");
	FullText::truncate_index("test_main_index");
	FullText::truncate_index("test_link_index");

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

BOOST_AUTO_TEST_SUITE_END();
