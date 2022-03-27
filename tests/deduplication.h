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

#include "hash_table_helper/hash_table_helper.h"
#include "api/api.h"
#include "json.hpp"

using json = nlohmann::json;

BOOST_AUTO_TEST_SUITE(deduplication)

BOOST_AUTO_TEST_CASE(deduplication) {

	unsigned long long initial_nodes_in_cluster = config::nodes_in_cluster;
	config::nodes_in_cluster = 1;
	config::node_id = 0;

	search_allocation::allocation *allocation = search_allocation::create_allocation();

	full_text::truncate_url_to_domain("main_index");
	full_text::truncate_index("test_main_index");

	hash_table_helper::truncate("test_main_index");

	// Index full text
	{
		common::sub_system *subsys = new common::sub_system();
		full_text::index_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-07", subsys);
	}

	hash_table::hash_table ht("test_main_index");
	full_text_index<full_text_record> index("test_main_index");

	{
		stringstream response_stream;
		api::search("The Wikipedia", ht, index, allocation, response_stream);

		string response = response_stream.str();

		json json_obj = json::parse(response);

		BOOST_CHECK(json_obj.contains("status"));
		BOOST_CHECK_EQUAL(json_obj["status"], "success");
		BOOST_CHECK_EQUAL(json_obj["total_url_links_found"], 0);

		BOOST_CHECK(json_obj.contains("results"));
		BOOST_CHECK_EQUAL(json_obj["results"].size(), config::result_limit);
	}

	// Reset.
	config::nodes_in_cluster = initial_nodes_in_cluster;
	config::node_id = 0;
}

BOOST_AUTO_TEST_CASE(api_search_deduplication_on_nodes) {

	search_allocation::allocation *allocation = search_allocation::create_allocation();

	full_text::truncate_url_to_domain("main_index");
	full_text::truncate_index("test_main_index");
	full_text::truncate_index("test_link_index");
	full_text::truncate_index("test_domain_link_index");

	hash_table_helper::truncate("test_main_index");
	hash_table_helper::truncate("test_link_index");
	hash_table_helper::truncate("test_domain_link_index");

	config::nodes_in_cluster = 1;
	config::node_id = 0;

	{
		// Index full text
		common::sub_system *subsys = new common::sub_system();
		full_text::index_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-01", subsys);
	}

	{
		// Index links
		url_to_domain *url_store = new url_to_domain("main_index");
		url_store->read();

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
		BOOST_CHECK_EQUAL(json_obj["results"].size(), 1);
	}

	search_allocation::delete_allocation(allocation);

	// Reset config.
	config::nodes_in_cluster = 1;
	config::node_id = 0;
}

BOOST_AUTO_TEST_CASE(api_search_deduplication) {

	search_allocation::allocation *allocation = search_allocation::create_allocation();

	full_text::truncate_url_to_domain("main_index");
	full_text::truncate_index("test_main_index");
	full_text::truncate_index("test_link_index");
	full_text::truncate_index("test_domain_link_index");

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
		url_to_domain *url_store = new url_to_domain("main_index");
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
		api::search("url2.com", ht, index, link_index, allocation, response_stream);

		string response = response_stream.str();

		json json_obj = json::parse(response);

		BOOST_CHECK(json_obj.contains("status"));
		BOOST_CHECK_EQUAL(json_obj["status"], "success");

		BOOST_CHECK(json_obj.contains("results"));
		BOOST_CHECK_EQUAL(json_obj["results"].size(), 19);
	}

	{
		stringstream response_stream;
		api::search_all("site:url2.com", ht, index, link_index, domain_link_index, allocation, response_stream);

		string response = response_stream.str();

		json json_obj = json::parse(response);

		BOOST_CHECK(json_obj.contains("status"));
		BOOST_CHECK_EQUAL(json_obj["status"], "success");

		BOOST_CHECK(json_obj.contains("results"));
		BOOST_CHECK_EQUAL(json_obj["results"].size(), 19);
	}

	search_allocation::delete_allocation(allocation);
	
}

BOOST_AUTO_TEST_SUITE_END()
