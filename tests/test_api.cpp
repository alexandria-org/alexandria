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

#include <boost/test/unit_test.hpp>
#include "full_text/full_text.h"
#include "hash_table_helper/hash_table_helper.h"
#include "full_text/url_to_domain.h"
#include "api/api.h"
#include "search_allocation/search_allocation.h"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;
using url_to_domain = full_text::url_to_domain;
using full_text::url_to_domain;

BOOST_AUTO_TEST_SUITE(api)

BOOST_AUTO_TEST_CASE(api_search) {

	search_allocation::allocation *allocation = search_allocation::create_allocation();

	full_text::truncate_url_to_domain("test_main_index");
	full_text::truncate_index("test_main_index");
	full_text::truncate_index("test_link_index");

	hash_table_helper::truncate("test_main_index");
	hash_table_helper::truncate("test_link_index");
	hash_table_helper::truncate("test_domain_link_index");

	// Index full text
	{
		common::sub_system *subsys = new common::sub_system();
		full_text::index_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-01", subsys);
	}

	{
		// Index links
		url_to_domain *url_store = new url_to_domain("test_main_index");
		url_store->read();

		BOOST_CHECK_EQUAL(url_store->size(), 8);

		common::sub_system *subsys = new common::sub_system();

		full_text::index_link_batch("test_link_index", "test_domain_link_index", "test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-01",
			subsys, url_store);
	}

	hash_table::hash_table ht("test_main_index");
	hash_table::hash_table link_ht("test_link_index");
	full_text_index<full_text_record> index("test_main_index");
	full_text_index<url_link::full_text_record> link_index("test_link_index");

	{
		stringstream response_stream;
		api::search("url1.com", ht, index, link_index, allocation, response_stream);

		string response = response_stream.str();

		json json_obj = json::parse(response);

		BOOST_CHECK(json_obj.contains("status"));
		BOOST_CHECK_EQUAL(json_obj["status"], "success");
		BOOST_CHECK_EQUAL(json_obj["total_found"], 1);
		BOOST_CHECK_EQUAL(json_obj["total_url_links_found"], 1);

		BOOST_CHECK(json_obj.contains("results"));
		BOOST_CHECK(json_obj["results"][0].contains("url"));
		BOOST_CHECK_EQUAL(json_obj["results"][0]["url"], "http://url1.com/test");
	}

	{
		stringstream response_stream;
		api::word_stats("Meta Description Text", index, link_index, ht.size(), link_ht.size(), response_stream);

		string response = response_stream.str();

		json json_obj = json::parse(response);

		BOOST_CHECK(json_obj.contains("status"));
		BOOST_CHECK_EQUAL(json_obj["status"], "success");

		BOOST_CHECK(json_obj.contains("time_ms"));

		BOOST_CHECK(json_obj.contains("index"));
		BOOST_CHECK(json_obj["index"].contains("words"));
		BOOST_CHECK_EQUAL(json_obj["index"]["words"]["meta"], 1.0);

		BOOST_CHECK(json_obj["index"].contains("total"));
		BOOST_CHECK_EQUAL(json_obj["index"]["total"], 8);
	}

	{
		stringstream response_stream;
		api::word_stats("more uniq", index, link_index, ht.size(), link_ht.size(), response_stream);

		string response = response_stream.str();

		json json_obj = json::parse(response);

		BOOST_CHECK(json_obj.contains("status"));
		BOOST_CHECK_EQUAL(json_obj["status"], "success");
		BOOST_CHECK(json_obj.contains("time_ms"));

		BOOST_CHECK(json_obj.contains("index"));
		BOOST_CHECK(json_obj["index"].contains("words"));
		BOOST_CHECK_EQUAL(json_obj["index"]["words"]["uniq"], 1.0/8.0);

		BOOST_CHECK(json_obj["index"].contains("total"));
		BOOST_CHECK_EQUAL(json_obj["index"]["total"], 8);
	}

	search_allocation::delete_allocation(allocation);
}

/*
 * Index without snippets and see that you get url hashes in binary format from /
 * */
BOOST_AUTO_TEST_CASE(api_search_no_snippets) {

	config::index_snippets = false;
	config::n_grams = 5;

	search_allocation::allocation *allocation = search_allocation::create_allocation();

	full_text::truncate_url_to_domain("main_index");
	full_text::truncate_index("test_main_index");
	full_text::truncate_index("test_link_index");

	hash_table_helper::truncate("test_main_index");
	hash_table_helper::truncate("test_link_index");
	hash_table_helper::truncate("test_domain_link_index");

	// Index full text
	{
		common::sub_system *subsys = new common::sub_system();
		full_text::index_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-01", subsys);
	}

	hash_table::hash_table ht("test_main_index");
	hash_table::hash_table link_ht("test_link_index");
	full_text_index<full_text_record> index("test_main_index");

	{
		stringstream response_stream;
		api::ids("url1.com h1 text", index, allocation, response_stream);

		string response = response_stream.str();

		const char *str = response.c_str();

		BOOST_CHECK_EQUAL(response.size(), 1 * sizeof(full_text_record));
		BOOST_CHECK_EQUAL(*((uint64_t *)&str[0]), URL("http://url1.com/test").hash());
	}

	{
		stringstream response_stream;
		api::ids("h1 text", index, allocation, response_stream);

		string response = response_stream.str();

		BOOST_CHECK_EQUAL(response.size(), 8 * sizeof(full_text_record));
	}

	search_allocation::delete_allocation(allocation);

	config::index_snippets = true;
}

BOOST_AUTO_TEST_CASE(api_search_compact) {

	search_allocation::allocation *allocation = search_allocation::create_allocation();

	full_text::truncate_url_to_domain("test_main_index");
	full_text::truncate_index("test_main_index");
	full_text::truncate_index("test_link_index");

	hash_table_helper::truncate("test_main_index");
	hash_table_helper::truncate("test_link_index");
	hash_table_helper::truncate("test_domain_link_index");

	common::sub_system *subsys = new common::sub_system();
	full_text::index_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-01", subsys);

	{
		// Index links
		url_to_domain *url_store = new url_to_domain("test_main_index");
		url_store->read();

		BOOST_CHECK_EQUAL(url_store->size(), 8);

		common::sub_system *subsys = new common::sub_system();

		full_text::index_link_batch("test_link_index", "test_domain_link_index", "test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-01",
			subsys, url_store);
	}

	hash_table::hash_table ht("test_main_index");
	hash_table::hash_table link_ht("test_link_index");
	full_text_index<full_text_record> index("test_main_index");
	full_text_index<url_link::full_text_record> link_index("test_link_index");

	{
		stringstream response_stream;
		api::search("url1.com", ht, index, link_index, allocation, response_stream);

		string response = response_stream.str();

		json json_obj = json::parse(response);

		BOOST_CHECK(json_obj.contains("status"));
		BOOST_CHECK_EQUAL(json_obj["status"], "success");

		BOOST_CHECK(json_obj.contains("results"));
		BOOST_CHECK(json_obj["results"][0].contains("url"));
		BOOST_CHECK_EQUAL(json_obj["results"][0]["url"], "http://url1.com/test");
	}

	{

		stringstream response_stream;
		api::word_stats("Meta Description Text", index, link_index, ht.size(), link_ht.size(), response_stream);

		string response = response_stream.str();

		json json_obj = json::parse(response);

		BOOST_CHECK(json_obj.contains("status"));
		BOOST_CHECK_EQUAL(json_obj["status"], "success");

		BOOST_CHECK(json_obj.contains("time_ms"));

		BOOST_CHECK(json_obj.contains("index"));
		BOOST_CHECK(json_obj["index"].contains("words"));
		BOOST_CHECK_EQUAL(json_obj["index"]["words"]["meta"], 1.0);

		BOOST_CHECK(json_obj["index"].contains("total"));
		BOOST_CHECK_EQUAL(json_obj["index"]["total"], 8);
	}

	{
		stringstream response_stream;
		api::word_stats("more uniq", index, link_index, ht.size(), link_ht.size(), response_stream);

		string response = response_stream.str();

		json json_obj = json::parse(response);

		BOOST_CHECK(json_obj.contains("status"));
		BOOST_CHECK_EQUAL(json_obj["status"], "success");
		BOOST_CHECK(json_obj.contains("time_ms"));

		BOOST_CHECK(json_obj.contains("index"));
		BOOST_CHECK(json_obj["index"].contains("words"));
		BOOST_CHECK_EQUAL(json_obj["index"]["words"]["uniq"], 1.0/8.0);

		BOOST_CHECK(json_obj["index"].contains("total"));
		BOOST_CHECK_EQUAL(json_obj["index"]["total"], 8);
	}

	search_allocation::delete_allocation(allocation);
}

BOOST_AUTO_TEST_CASE(api_search_with_domain_links) {

	search_allocation::allocation *allocation = search_allocation::create_allocation();

	full_text::truncate_url_to_domain("test_main_index");
	full_text::truncate_index("test_main_index");
	full_text::truncate_index("test_link_index");
	full_text::truncate_index("test_domain_link_index");

	hash_table_helper::truncate("test_main_index");
	hash_table_helper::truncate("test_link_index");
	hash_table_helper::truncate("test_domain_link_index");

	{
		// Index full text
		common::sub_system *subsys = new common::sub_system();
		full_text::index_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-01", subsys);
	}

	{
		// Index links
		url_to_domain *url_store = new url_to_domain("test_main_index");
		url_store->read();

		BOOST_CHECK_EQUAL(url_store->size(), 8);

		common::sub_system *subsys = new common::sub_system();

		full_text::index_link_batch("test_link_index", "test_domain_link_index", "test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-01",
			subsys, url_store);
	}

	hash_table::hash_table ht("test_main_index");
	hash_table::hash_table link_ht("test_link_index");
	full_text_index<full_text_record> index("test_main_index");
	full_text_index<url_link::full_text_record> link_index("test_link_index");
	full_text_index<domain_link::full_text_record> domain_link_index("test_domain_link_index");

	{
		stringstream response_stream;
		api::search("url1.com", ht, index, link_index, domain_link_index, allocation, response_stream);

		string response = response_stream.str();

		json json_obj = json::parse(response);

		BOOST_CHECK(json_obj.contains("status"));
		BOOST_CHECK_EQUAL(json_obj["status"], "success");
		BOOST_CHECK_EQUAL(json_obj["total_found"], 1);
		BOOST_CHECK_EQUAL(json_obj["total_domain_links_found"], 2);

		BOOST_CHECK(json_obj.contains("results"));
		BOOST_CHECK(json_obj["results"][0].contains("url"));
		BOOST_CHECK_EQUAL(json_obj["results"][0]["url"], "http://url1.com/test");
	}

	search_allocation::delete_allocation(allocation);
}

BOOST_AUTO_TEST_CASE(many_links) {

	search_allocation::allocation *allocation = search_allocation::create_allocation();

	full_text::truncate_url_to_domain("test_main_index");
	full_text::truncate_index("test_main_index");
	full_text::truncate_index("test_link_index");

	hash_table_helper::truncate("test_main_index");
	hash_table_helper::truncate("test_link_index");
	hash_table_helper::truncate("test_domain_link_index");

	{
		// Index full text
		common::sub_system *subsys = new common::sub_system();
		full_text::index_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-06", subsys);
	}

	{
		// Index links
		url_to_domain *url_store = new url_to_domain("test_main_index");
		url_store->read();

		common::sub_system *subsys = new common::sub_system();

		full_text::index_link_batch("test_link_index", "test_domain_link_index", "test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-06",
			subsys, url_store);
	}

	hash_table::hash_table ht("test_main_index");
	full_text_index<full_text_record> index("test_main_index");
	full_text_index<url_link::full_text_record> link_index("test_link_index");
	full_text_index<domain_link::full_text_record> domain_link_index("test_domain_link_index");

	{
		stringstream response_stream;
		api::search("url1.com", ht, index, link_index, domain_link_index, allocation, response_stream);

		string response = response_stream.str();

		json json_obj = json::parse(response);

		BOOST_CHECK(json_obj.contains("status"));
		BOOST_CHECK_EQUAL(json_obj["status"], "success");
		BOOST_CHECK_EQUAL(json_obj["total_found"], 6);
		BOOST_CHECK_EQUAL(json_obj["link_url_matches"], 15);
	}

	search_allocation::delete_allocation(allocation);

}

BOOST_AUTO_TEST_CASE(api_word_stats) {

	full_text::truncate_url_to_domain("test_main_index");
	full_text::truncate_index("test_main_index");
	full_text::truncate_index("test_link_index");

	hash_table_helper::truncate("test_main_index");
	hash_table_helper::truncate("test_link_index");
	hash_table_helper::truncate("test_domain_link_index");

	{
		// Index full text
		common::sub_system *subsys = new common::sub_system();
		full_text::index_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-02", subsys);
	}

	{
		// Index links
		url_to_domain *url_store = new url_to_domain("test_main_index");
		url_store->read();

		BOOST_CHECK_EQUAL(url_store->size(), 8);

		common::sub_system *subsys = new common::sub_system();

		full_text::index_link_batch("test_link_index", "test_domain_link_index", "test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-02",
			subsys, url_store);
	}

	hash_table::hash_table ht("test_main_index");
	hash_table::hash_table link_ht("test_link_index");
	full_text_index<full_text_record> index("test_main_index");
	full_text_index<url_link::full_text_record> link_index("test_link_index");

	{
		stringstream response_stream;
		api::word_stats("uniq", index, link_index, ht.size(), link_ht.size(), response_stream);

		string response = response_stream.str();

		json json_obj = json::parse(response);

		BOOST_CHECK(json_obj.contains("status"));
		BOOST_CHECK_EQUAL(json_obj["status"], "success");

		BOOST_CHECK(json_obj.contains("time_ms"));

		BOOST_CHECK(json_obj.contains("index"));
		BOOST_CHECK(json_obj["index"].contains("words"));
		BOOST_CHECK_EQUAL(json_obj["index"]["words"]["uniq"], 2.0/8.0);

		BOOST_CHECK(json_obj["index"].contains("total"));
		BOOST_CHECK_EQUAL(json_obj["index"]["total"], 8);
	}

	{
		stringstream response_stream;
		api::word_stats("test07.links.gz", index, link_index, ht.size(), link_ht.size(), response_stream);

		string response = response_stream.str();

		json json_obj = json::parse(response);

		BOOST_CHECK(json_obj.contains("status"));
		BOOST_CHECK_EQUAL(json_obj["status"], "success");

		BOOST_CHECK(json_obj.contains("time_ms"));
	}
}

BOOST_AUTO_TEST_CASE(api_hash_table) {

	full_text::truncate_url_to_domain("test_main_index");
	full_text::truncate_index("test_main_index");
	full_text::truncate_index("test_link_index");

	hash_table_helper::truncate("test_main_index");
	hash_table_helper::truncate("test_link_index");
	hash_table_helper::truncate("test_domain_link_index");

	{
		// Index full text
		common::sub_system *subsys = new common::sub_system();
		full_text::index_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-04", subsys);
	}

	{
		// Index links
		url_to_domain *url_store = new url_to_domain("test_main_index");
		url_store->read();

		common::sub_system *subsys = new common::sub_system();

		full_text::index_link_batch("test_link_index", "test_domain_link_index", "test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-04",
			subsys, url_store);
	}

	hash_table::hash_table ht("test_main_index");

	{
		stringstream response_stream;
		api::url("http://url1.com/my_test_url", ht, response_stream);

		string response = response_stream.str();

		json json_obj = json::parse(response);

		BOOST_CHECK(json_obj.contains("status"));
		BOOST_CHECK_EQUAL(json_obj["status"], "success");

		BOOST_CHECK(json_obj.contains("time_ms"));
		BOOST_CHECK(json_obj.contains("response"));

		BOOST_CHECK_EQUAL(json_obj["response"], "http://url1.com/my_test_url	test test		test test test test		ALEXANDRIA-TEST-04");
	}

	{
		stringstream response_stream;
		api::url("http://non-existing-url.com", ht, response_stream);

		string response = response_stream.str();

		json json_obj = json::parse(response);

		BOOST_CHECK(json_obj.contains("status"));
		BOOST_CHECK_EQUAL(json_obj["status"], "success");

		BOOST_CHECK(json_obj.contains("time_ms"));
		BOOST_CHECK(json_obj.contains("response"));

		BOOST_CHECK_EQUAL(json_obj["response"], "");
	}
}

BOOST_AUTO_TEST_CASE(api_no_text) {

	// This test relies on remote server to be running. Should be implemented propely...
	return;

	config::index_snippets = true;
	config::index_text = false;

	search_allocation::allocation *allocation = search_allocation::create_allocation();

	full_text::truncate_url_to_domain("test_main_index");
	full_text::truncate_index("test_main_index");
	full_text::truncate_index("test_link_index");

	hash_table_helper::truncate("test_main_index");
	hash_table_helper::truncate("test_link_index");
	hash_table_helper::truncate("test_domain_link_index");

	{
		// Index full text
		common::sub_system *subsys = new common::sub_system();
		full_text::index_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-09", subsys);
	}

	{
		// Index links
		url_to_domain *url_store = new url_to_domain("test_main_index");
		url_store->read();

		common::sub_system *subsys = new common::sub_system();

		full_text::index_link_batch("test_link_index", "test_domain_link_index", "test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-09",
			subsys, url_store);
	}

	hash_table::hash_table ht("test_main_index");
	full_text_index<url_link::full_text_record> link_index("test_link_index");
	full_text_index<domain_link::full_text_record> domain_link_index("test_domain_link_index");

	{
		stringstream response_stream;
		api::search_remote("1936.com", ht, link_index, domain_link_index, allocation, response_stream);

		string response = response_stream.str();

		json json_obj = json::parse(response);

		BOOST_CHECK(json_obj.contains("status"));
		BOOST_CHECK_EQUAL(json_obj["status"], "success");

		BOOST_CHECK(json_obj.contains("time_ms"));
		BOOST_CHECK(json_obj.contains("results"));

		BOOST_CHECK_EQUAL(json_obj["results"][0]["url"], "http://1936.com/");
		BOOST_CHECK(json_obj["results"][0]["score"] > 4.0);
	}

	search_allocation::delete_allocation(allocation);

	config::index_text = true;
}

BOOST_AUTO_TEST_SUITE_END()
