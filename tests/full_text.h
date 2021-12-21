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

#include "full_text/FullText.h"
#include "parser/URL.h"
#include "hash_table/HashTable.h"
#include "hash_table/HashTableHelper.h"
#include "full_text/FullText.h"
#include "full_text/UrlToDomain.h"
#include "full_text/FullTextIndex.h"
#include "full_text/FullTextIndexerRunner.h"
#include "link_index/LinkIndexerRunner.h"
#include "search_engine/SearchEngine.h"

BOOST_AUTO_TEST_SUITE(full_text)

BOOST_AUTO_TEST_CASE(download_batch) {

	vector<string> files = FullText::download_batch("ALEXANDRIA-TEST-01", 100, 0);

	BOOST_CHECK_EQUAL(files.size(), 8);
}

BOOST_AUTO_TEST_CASE(make_partition_from_files) {
	{
		auto partition = FullText::make_partition_from_files({"file1", "file2", "file3", "file4"}, 1, 3);
		BOOST_CHECK_EQUAL(partition.size(), 1);
		BOOST_CHECK_EQUAL(partition[0], "file2");
	}

	{
		auto partition = FullText::make_partition_from_files({"file1", "file2", "file3", "file4"}, 0, 3);
		BOOST_CHECK_EQUAL(partition.size(), 2);
		BOOST_CHECK_EQUAL(partition[0], "file1");
		BOOST_CHECK_EQUAL(partition[1], "file4");
	}
}

BOOST_AUTO_TEST_CASE(indexer) {

	SearchAllocation::Allocation *allocation = SearchAllocation::create_allocation();

	FullText::truncate_url_to_domain("main_index");
	FullText::truncate_index("test_main_index");
	FullText::truncate_index("test_link_index");

	HashTableHelper::truncate("test_main_index");
	HashTableHelper::truncate("test_link_index");
	HashTableHelper::truncate("test_domain_link_index");

	{
		// Index full text
		FullText::index_single_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-01");
	}

	{
		// Index links
		FullText::index_single_link_batch("test_link_index", "test_domain_link_index", "test_link_index", "test_domain_link_index",
			"ALEXANDRIA-TEST-01");
	}

	{
		// Count elements in hash tables.
		HashTable hash_table("test_main_index");
		HashTable hash_table_link("test_link_index");

		BOOST_CHECK_EQUAL(hash_table.size(), 8);

		// Make searches.
		vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("test_main_index");
		vector<FullTextIndex<LinkFullTextRecord> *> link_index_array = FullText::create_index_array<LinkFullTextRecord>("test_link_index");

		const string query = "Url1.com";
		struct SearchMetric metric;

		vector<LinkFullTextRecord> links = SearchEngine::search<LinkFullTextRecord>(allocation->link_storage, link_index_array, {}, {}, query,
			1000, metric);
		vector<FullTextRecord> results = SearchEngine::search_deduplicate(allocation->storage, index_array, links, {}, query, 1000, metric);

		BOOST_CHECK_EQUAL(links.size(), 1);
		BOOST_CHECK_EQUAL(results.size(), 1);
		BOOST_CHECK_EQUAL(metric.m_total_found, 1);
		BOOST_CHECK_EQUAL(metric.m_link_domain_matches, 0);
		BOOST_CHECK_EQUAL(metric.m_link_url_matches, 1);
		BOOST_CHECK_EQUAL(results[0].m_value, URL("http://url1.com/test").hash());

		FullText::delete_index_array<FullTextRecord>(index_array);
		FullText::delete_index_array<LinkFullTextRecord>(link_index_array);
	}

	SearchAllocation::delete_allocation(allocation);
}

BOOST_AUTO_TEST_CASE(indexer_multiple_link_batches) {

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

		FullText::index_single_link_batch("test_link_index", "test_domain_link_index", "test_link_index", "test_domain_link_index",
			"ALEXANDRIA-TEST-01");

	}

	{
		// Index more links and see if they get added with deduplication.
		FullText::index_single_link_batch("test_link_index", "test_domain_link_index", "test_link_index", "test_domain_link_index",
			"ALEXANDRIA-TEST-02");
	}

	{
		// Count elements in hash tables.
		HashTable hash_table("test_main_index");
		HashTable hash_table_link("test_link_index");

		BOOST_CHECK_EQUAL(hash_table.size(), 8);

		// Make searches.
		vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("test_main_index");
		vector<FullTextIndex<LinkFullTextRecord> *> link_index_array = FullText::create_index_array<LinkFullTextRecord>("test_link_index");

		const string query = "Url8.com";
		struct SearchMetric metric;

		{
			vector<LinkFullTextRecord> links = SearchEngine::search<LinkFullTextRecord>(allocation->link_storage, link_index_array, {}, {}, query,
				1000, metric);
			vector<FullTextRecord> results = SearchEngine::search_deduplicate(allocation->storage, index_array, links, {}, query, 1000, metric);

			BOOST_CHECK_EQUAL(links.size(), 3);
			BOOST_CHECK_EQUAL(results.size(), 1);
			BOOST_CHECK_EQUAL(metric.m_total_found, 1);
			BOOST_CHECK_EQUAL(metric.m_link_domain_matches, 0);
			BOOST_CHECK_EQUAL(metric.m_link_url_matches, 2);
			BOOST_CHECK_EQUAL(results[0].m_value, URL("http://url8.com/test").hash());
		}

		{
			vector<LinkFullTextRecord> links = SearchEngine::search<LinkFullTextRecord>(allocation->link_storage, link_index_array, {}, {}, query,
				1, metric);

			BOOST_CHECK_EQUAL(links.size(), 1);
			BOOST_CHECK_EQUAL(links[0].m_value, URL("http://url8.com/").link_hash(URL("http://url7.com/test"), "Link to url7.com from url8.com"));
		}

		FullText::delete_index_array<FullTextRecord>(index_array);
		FullText::delete_index_array<LinkFullTextRecord>(link_index_array);
	}

	SearchAllocation::delete_allocation(allocation);
}

BOOST_AUTO_TEST_CASE(domain_links) {

	SearchAllocation::Allocation *allocation = SearchAllocation::create_allocation();

	FullText::truncate_url_to_domain("main_index");
	FullText::truncate_index("test_link_index");
	FullText::truncate_index("test_domain_link_index");

	HashTableHelper::truncate("test_main_index");
	HashTableHelper::truncate("test_link_index");
	HashTableHelper::truncate("test_domain_link_index");

	{
		// Index full text
		SubSystem *sub_system = new SubSystem();
		FullText::index_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-05", sub_system);
	}

	{
		// Index links
		UrlToDomain *url_to_domain = new UrlToDomain("main_index");
		url_to_domain->read();

		BOOST_CHECK_EQUAL(url_to_domain->size(), 10);

		SubSystem *sub_system = new SubSystem();

		FullText::index_link_batch("test_link_index", "test_domain_link_index", "test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-05",
			sub_system, url_to_domain);
	}

	{
		// Count elements in hash tables.
		HashTable hash_table("test_main_index");
		HashTable hash_table_link("test_link_index");
		HashTable hash_table_domain_link("test_domain_link_index");

		BOOST_CHECK_EQUAL(hash_table.size(), 10);

		// Make searches.
		vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("test_main_index");
		vector<FullTextIndex<LinkFullTextRecord> *> link_index_array = FullText::create_index_array<LinkFullTextRecord>("test_link_index");
		vector<FullTextIndex<DomainLink::FullTextRecord> *> domain_link_index_array =
			FullText::create_index_array<DomainLink::FullTextRecord>("test_domain_link_index");

		{
			const string query = "Testing the test from test04.links.gz";
			struct SearchMetric metric;

			vector<DomainLink::FullTextRecord> links = SearchEngine::search<DomainLink::FullTextRecord>(allocation->domain_link_storage,
				domain_link_index_array, {}, {}, query, 1000, metric);

			BOOST_CHECK_EQUAL(links.size(), 1);
			BOOST_CHECK_EQUAL(links[0].m_value, URL("http://linksource4.com/").domain_link_hash(URL("http://url5.com/test"),
				"Testing the test from test04.links.gz"));
		}

		{
			const string query = "Url6.com";
			struct SearchMetric metric;

			vector<LinkFullTextRecord> links = SearchEngine::search<LinkFullTextRecord>(allocation->link_storage, link_index_array, {}, {}, query,
				1000, metric);
			vector<DomainLink::FullTextRecord> domain_links = SearchEngine::search<DomainLink::FullTextRecord>(allocation->domain_link_storage,
				domain_link_index_array, {}, {}, query, 1000, metric);

			vector<FullTextRecord> results_no_domainlinks = SearchEngine::search_deduplicate(allocation->storage, index_array, links, {}, query,
				1000, metric);
			vector<FullTextRecord> results = SearchEngine::search_deduplicate(allocation->storage, index_array, links, domain_links, query, 1000,
				metric);

			BOOST_CHECK_EQUAL(links.size(), 3);
			BOOST_CHECK_EQUAL(domain_links.size(), 2);
			BOOST_CHECK_EQUAL(results.size(), 2);
			BOOST_CHECK_EQUAL(results_no_domainlinks.size(), 2);

			bool has_link = false;
			for (const LinkFullTextRecord &link : links) {
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

		FullText::delete_index_array<FullTextRecord>(index_array);
		FullText::delete_index_array<LinkFullTextRecord>(link_index_array);
		FullText::delete_index_array<DomainLink::FullTextRecord>(domain_link_index_array);
	}

	SearchAllocation::delete_allocation(allocation);
}

BOOST_AUTO_TEST_CASE(indexer_test_deduplication) {

	unsigned long long initial_nodes_in_cluster = Config::nodes_in_cluster;
	Config::nodes_in_cluster = 1;
	Config::node_id = 0;

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
		FullText::index_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-03", sub_system);
	}

	{
		// Count elements in hash tables.
		HashTable hash_table("test_main_index");

		BOOST_CHECK_EQUAL(hash_table.size(), 8);

		// Make searches.
		vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("test_main_index");

		const string query = "my first url";
		struct SearchMetric metric;

		vector<FullTextRecord> results = SearchEngine::search<FullTextRecord>(allocation->storage, index_array, {}, {}, query, 1000, metric);

		BOOST_CHECK_EQUAL(results.size(), 1);
		BOOST_CHECK_EQUAL(metric.m_total_found, 1);
		BOOST_CHECK_EQUAL(results[0].m_value, URL("http://url1.com/test").hash());

		results = SearchEngine::search<FullTextRecord>(allocation->storage, index_array, {}, {}, "my second url", 1000, metric);
		BOOST_CHECK_EQUAL(results.size(), 1);
		BOOST_CHECK_EQUAL(metric.m_total_found, 1);
		BOOST_CHECK_EQUAL(results[0].m_value, URL("http://url2.com/test").hash());

		FullText::delete_index_array<FullTextRecord>(index_array);
	}

	SearchAllocation::delete_allocation(allocation);

	// Reset.
	Config::nodes_in_cluster = initial_nodes_in_cluster;
	Config::node_id = 0;
}

BOOST_AUTO_TEST_SUITE_END()
