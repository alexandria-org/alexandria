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
#include "api/api.h"
#include "URL.h"
#include "hash_table/hash_table.h"
#include "hash_table_helper/hash_table_helper.h"
#include "full_text/full_text.h"
#include "full_text/url_to_domain.h"
#include "full_text/full_text_index.h"
#include "full_text/full_text_indexer_runner.h"
#include "search_engine/search_engine.h"

#include "json.hpp"

using json = nlohmann::json;
using full_text_record = full_text::full_text_record;

BOOST_AUTO_TEST_SUITE(test_full_text)

BOOST_AUTO_TEST_CASE(download_batch) {

	vector<string> files = full_text::download_batch("ALEXANDRIA-TEST-01", 100, 0);

	BOOST_CHECK_EQUAL(files.size(), 8);
}

BOOST_AUTO_TEST_CASE(indexer) {

	search_allocation::allocation *allocation = search_allocation::create_allocation();

	full_text::truncate_url_to_domain("test_main_index");
	full_text::truncate_index("test_main_index");
	full_text::truncate_index("test_link_index");

	hash_table_helper::truncate("test_main_index");
	hash_table_helper::truncate("test_link_index");
	hash_table_helper::truncate("test_domain_link_index");

	{
		// Index full text
		full_text::index_single_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-01");
	}

	{
		// Index links
		full_text::url_to_domain *url_store = new full_text::url_to_domain("test_main_index");
		full_text::index_single_link_batch("test_link_index", "test_domain_link_index", "test_link_index", "test_domain_link_index",
			"ALEXANDRIA-TEST-01", url_store);
	}

	{
		// Count elements in hash tables.
		hash_table::hash_table ht("test_main_index");
		hash_table::hash_table ht_link("test_link_index");

		BOOST_CHECK_EQUAL(ht.size(), 8);

		// Make searches.
		full_text::full_text_index<full_text_record> index("test_main_index");
		full_text::full_text_index<url_link::full_text_record> link_index("test_link_index");

		const string query = "Url1.com";
		struct full_text::search_metric metric;

		vector<url_link::full_text_record> links = search_engine::search<url_link::full_text_record>(allocation->link_storage, link_index, {}, {}, query,
			1000, metric);
		vector<full_text_record> results = search_engine::search_deduplicate(allocation->record_storage, index, links, {}, query, 1000, metric);

		BOOST_CHECK_EQUAL(links.size(), 1);
		BOOST_CHECK_EQUAL(results.size(), 1);
		BOOST_CHECK_EQUAL(metric.m_total_found, 1);
		BOOST_CHECK_EQUAL(metric.m_link_domain_matches, 0);
		BOOST_CHECK_EQUAL(metric.m_link_url_matches, 1);
		BOOST_CHECK_EQUAL(results[0].m_value, URL("http://url1.com/test").hash());
	}

	search_allocation::delete_allocation(allocation);
}

BOOST_AUTO_TEST_CASE(indexer_multiple_link_batches) {

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

	full_text::url_to_domain *url_store = new full_text::url_to_domain("test_main_index");
	url_store->read();
	BOOST_CHECK_EQUAL(url_store->size(), 8);

	{
		// Index links
		full_text::index_single_link_batch("test_link_index", "test_domain_link_index", "test_link_index", "test_domain_link_index",
			"ALEXANDRIA-TEST-01", url_store);

	}

	{
		// Index more links and see if they get added with deduplication.
		full_text::index_single_link_batch("test_link_index", "test_domain_link_index", "test_link_index", "test_domain_link_index",
			"ALEXANDRIA-TEST-02", url_store);
	}

	{
		// Count elements in hash tables.
		hash_table::hash_table ht("test_main_index");
		hash_table::hash_table ht_link("test_link_index");

		BOOST_CHECK_EQUAL(ht.size(), 8);

		// Make searches.
		full_text::full_text_index<full_text_record> index("test_main_index");
		full_text::full_text_index<url_link::full_text_record> link_index("test_link_index");

		const string query = "Url8.com";
		struct full_text::search_metric metric;

		{
			vector<url_link::full_text_record> links = search_engine::search<url_link::full_text_record>(allocation->link_storage, link_index, {}, {}, query,
				1000, metric);
			vector<full_text_record> results = search_engine::search_deduplicate(allocation->record_storage, index, links, {}, query, 1000, metric);

			BOOST_CHECK_EQUAL(links.size(), 3);
			BOOST_CHECK_EQUAL(results.size(), 1);
			BOOST_CHECK_EQUAL(metric.m_total_found, 1);
			BOOST_CHECK_EQUAL(metric.m_link_domain_matches, 0);
			BOOST_CHECK_EQUAL(metric.m_link_url_matches, 2);
			BOOST_CHECK_EQUAL(results[0].m_value, URL("http://url8.com/test").hash());
		}

		{
			vector<url_link::full_text_record> links = search_engine::search<url_link::full_text_record>(allocation->link_storage, link_index, {}, {}, query,
				1, metric);

			BOOST_CHECK_EQUAL(links.size(), 1);
			BOOST_CHECK_EQUAL(links[0].m_value, URL("http://url8.com/").link_hash(URL("http://url7.com/test"), "Link to url7.com from url8.com"));
		}
	}

	search_allocation::delete_allocation(allocation);
}

BOOST_AUTO_TEST_CASE(domain_links) {

	search_allocation::allocation *allocation = search_allocation::create_allocation();

	full_text::truncate_url_to_domain("test_main_index");
	full_text::truncate_index("test_link_index");
	full_text::truncate_index("test_domain_link_index");

	hash_table_helper::truncate("test_main_index");
	hash_table_helper::truncate("test_link_index");
	hash_table_helper::truncate("test_domain_link_index");

	{
		// Index full text
		common::sub_system *subsys = new common::sub_system();
		full_text::index_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-05", subsys);
	}

	{
		// Index links
		full_text::url_to_domain *url_store = new full_text::url_to_domain("test_main_index");
		url_store->read();

		BOOST_CHECK_EQUAL(url_store->size(), 10);

		common::sub_system *subsys = new common::sub_system();

		full_text::index_link_batch("test_link_index", "test_domain_link_index", "test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-05",
			subsys, url_store);
	}

	{
		// Count elements in hash tables.
		hash_table::hash_table ht("test_main_index");
		hash_table::hash_table ht_link("test_link_index");
		hash_table::hash_table ht_domain_link("test_domain_link_index");

		BOOST_CHECK_EQUAL(ht.size(), 10);

		// Make searches.
		full_text::full_text_index<full_text_record> index("test_main_index");
		full_text::full_text_index<url_link::full_text_record> link_index("test_link_index");
		full_text::full_text_index<domain_link::full_text_record> domain_link_index("test_domain_link_index");

		{
			const string query = "Testing the test from test04.links.gz";
			struct full_text::search_metric metric;

			vector<domain_link::full_text_record> links = search_engine::search<domain_link::full_text_record>(allocation->domain_link_storage,
				domain_link_index, {}, {}, query, 1000, metric);

			BOOST_CHECK_EQUAL(links.size(), 1);
			BOOST_CHECK_EQUAL(links[0].m_value, URL("http://linksource4.com/").domain_link_hash(URL("http://url5.com/test"),
				"Testing the test from test04.links.gz"));
		}

		{
			const string query = "Url6.com";
			struct full_text::search_metric metric;

			vector<url_link::full_text_record> links = search_engine::search<url_link::full_text_record>(allocation->link_storage, link_index, {}, {}, query,
				1000, metric);
			vector<domain_link::full_text_record> domain_links = search_engine::search<domain_link::full_text_record>(allocation->domain_link_storage,
				domain_link_index, {}, {}, query, 1000, metric);

			vector<full_text_record> results_no_domainlinks = search_engine::search_deduplicate(allocation->record_storage, index, links, {}, query,
				1000, metric);
			vector<full_text_record> results = search_engine::search_deduplicate(allocation->record_storage, index, links, domain_links, query, 1000,
				metric);

			BOOST_CHECK_EQUAL(links.size(), 3);
			BOOST_CHECK_EQUAL(domain_links.size(), 2);
			BOOST_CHECK_EQUAL(results.size(), 2);
			BOOST_CHECK_EQUAL(results_no_domainlinks.size(), 2);

			bool has_link = false;
			for (const url_link::full_text_record &link : links) {
				if (link.m_value == URL("http://url6.com/").link_hash(URL("http://url7.com/test"), "Link to url7.com from url6.com")) {
					has_link = true;
				}
			}
			BOOST_CHECK(has_link);

			const uint64_t hash1 = URL("http://url6.com/").domain_link_hash(URL("http://url7.com/test"), "Link to url7.com from url6.com");
			const uint64_t hash2 = URL("http://url8.com/").domain_link_hash(URL("http://url6.com/test"), "Link to url6.com");
			BOOST_CHECK(domain_links[0].m_value == hash1 || domain_links[0].m_value == hash2);
			BOOST_CHECK(domain_links[1].m_value == hash1 || domain_links[1].m_value == hash2);

			BOOST_CHECK(results_no_domainlinks[0].m_score < results[0].m_score);

		}
	}

	search_allocation::delete_allocation(allocation);
}

BOOST_AUTO_TEST_CASE(indexer_test_deduplication) {

	unsigned long long initial_nodes_in_cluster = config::nodes_in_cluster;
	config::nodes_in_cluster = 1;
	config::node_id = 0;

	search_allocation::allocation *allocation = search_allocation::create_allocation();

	full_text::truncate_url_to_domain("main_index");
	full_text::truncate_index("test_main_index");
	full_text::truncate_index("test_link_index");

	hash_table_helper::truncate("test_main_index");
	hash_table_helper::truncate("test_link_index");
	hash_table_helper::truncate("test_domain_link_index");

	{
		// Index full text
		common::sub_system *subsys = new common::sub_system();
		full_text::index_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-03", subsys);
	}

	{
		// Count elements in hash tables.
		hash_table::hash_table ht("test_main_index");

		BOOST_CHECK_EQUAL(ht.size(), 8);

		// Make searches.
		full_text::full_text_index<full_text_record> index("test_main_index");

		const string query = "my first url";
		struct full_text::search_metric metric;

		vector<full_text_record> results = search_engine::search<full_text_record>(allocation->record_storage, index, {}, {}, query, 1000, metric);

		BOOST_CHECK_EQUAL(results.size(), 1);
		BOOST_CHECK_EQUAL(metric.m_total_found, 1);
		BOOST_CHECK_EQUAL(results[0].m_value, URL("http://url1.com/test").hash());

		results = search_engine::search<full_text_record>(allocation->record_storage, index, {}, {}, "my second url", 1000, metric);
		BOOST_CHECK_EQUAL(results.size(), 1);
		BOOST_CHECK_EQUAL(metric.m_total_found, 1);
		BOOST_CHECK_EQUAL(results[0].m_value, URL("http://url2.com/test").hash());
	}

	search_allocation::delete_allocation(allocation);

	// Reset.
	config::nodes_in_cluster = initial_nodes_in_cluster;
	config::node_id = 0;
}

BOOST_AUTO_TEST_CASE(shard_buffer_size) {

	size_t initial_buffer_len = config::ft_shard_builder_buffer_len;
	config::ft_shard_builder_buffer_len = 48;

	search_allocation::allocation *allocation = search_allocation::create_allocation();

	full_text::truncate_url_to_domain("main_index");
	full_text::truncate_index("test_main_index");

	hash_table_helper::truncate("test_main_index");

	// Index full text twice
	{
		common::sub_system *subsys = new common::sub_system();
		full_text::index_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-08", subsys);
		full_text::index_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-08", subsys);
	}

	hash_table::hash_table ht("test_main_index");
	full_text::full_text_index<full_text_record> index("test_main_index");

	{
		stringstream response_stream;
		api::search("site:en.wikipedia.org Wikipedia", ht, index, allocation, response_stream);

		string response = response_stream.str();

		json json_obj = json::parse(response);

		BOOST_CHECK(json_obj.contains("status"));
		BOOST_CHECK_EQUAL(json_obj["status"], "success");
		BOOST_CHECK_EQUAL(json_obj["total_url_links_found"], 0);

		BOOST_CHECK(json_obj.contains("results"));
		BOOST_CHECK_EQUAL(json_obj["results"].size(), 1000);
	}

	search_allocation::delete_allocation(allocation);

	config::ft_shard_builder_buffer_len = initial_buffer_len;
}

BOOST_AUTO_TEST_CASE(is_indexed) {

	search_allocation::allocation *allocation = search_allocation::create_allocation();

	full_text::truncate_url_to_domain("main_index");
	full_text::truncate_index("main_index");

	// Index full text
	{
		common::sub_system *subsys = new common::sub_system();
		full_text::index_batch("main_index", "main_index", "ALEXANDRIA-TEST-08", subsys);
	}

	BOOST_CHECK(full_text::is_indexed());

	search_allocation::delete_allocation(allocation);
}

BOOST_AUTO_TEST_SUITE_END()
