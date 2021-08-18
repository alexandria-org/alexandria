
#include "full_text/FullText.h"
#include "parser/URL.h"
#include "hash_table/HashTable.h"
#include "hash_table/HashTableHelper.h"
#include "full_text/FullText.h"
#include "full_text/UrlToDomain.h"
#include "full_text/FullTextIndex.h"
#include "full_text/FullTextIndexerRunner.h"
#include "link_index/LinkIndexerRunner.h"
#include "link_index/LinkIndexerRunnerNew.h"
#include "search_engine/SearchEngine.h"

BOOST_AUTO_TEST_SUITE(full_text)

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

BOOST_AUTO_TEST_CASE(indexer_runner_1) {

	{
		FullTextIndexerRunner indexer("test_db1", "test_db1", "CC-MAIN-2021-17");
		indexer.truncate();
		indexer.index_text("http://aciedd.org/fixing-solar-panels/	Fixing Solar Panels ‚Äì blog	blog		Menu Home Search for: Posted in General Fixing Solar Panels Author: Holly Montgomery Published Date: December 24, 2020 Leave a Comment on Fixing Solar Panels Complement your renewable power project with Perfection fashionable solar panel assistance structures. If you live in an region that receives a lot of snow in the winter, becoming able to easily sweep the snow off of your solar panels is a major comfort. If your solar panel contractor advises you that horizontal solar panels are the greatest selection for your solar wants, you do not need to have a particular inverter. The Solar PV panels are then clamped to the rails, keeping the panels really close to the roof to decrease wind loading. For 1 point, solar panels require to face either south or west to get direct sunlight. Once you have bought your solar panel you will need to have to determine on a safe fixing method, our extensive variety of permanent and non permane");
		indexer.merge();
		indexer.sort();

		FullTextIndex<FullTextRecord> fti("test_db1");

		struct SearchMetric metric;
		vector<FullTextRecord> result = SearchEngine::search(fti.shards(), {}, "permanent", 1000, metric);

		BOOST_CHECK_EQUAL(result.size(), 1);
		BOOST_CHECK_EQUAL(result[0].m_value, URL("http://aciedd.org/fixing-solar-panels/").hash());
	}

	{
		FullTextIndexerRunner indexer("test_db2", "test_db2", "CC-MAIN-2021-17");
		indexer.truncate();
		indexer.index_text("http://example.com	title	h1	meta	Hej hopp josef");
		indexer.index_text("http://example2.com	title	h1	meta	jag heter test");
		indexer.index_text("http://example3.com	title	h1	meta	ett två åäö jag testar");
		indexer.merge();
		indexer.sort();

		FullTextIndex<FullTextRecord> fti("test_db2");

		struct SearchMetric metric;
		vector<FullTextRecord> result = SearchEngine::search(fti.shards(), {}, "josef", 1000, metric);

		BOOST_CHECK_EQUAL(result.size(), 1);
		BOOST_CHECK_EQUAL(result[0].m_value, URL("http://example.com").hash());

		result = SearchEngine::search(fti.shards(), {}, "åäö", 1000, metric);
		BOOST_CHECK_EQUAL(result.size(), 1);
		BOOST_CHECK_EQUAL(result[0].m_value, URL("http://example3.com").hash());

		result = SearchEngine::search(fti.shards(), {}, "testar", 1000, metric);
		BOOST_CHECK_EQUAL(result.size(), 1);
		BOOST_CHECK_EQUAL(result[0].m_value, URL("http://example3.com").hash());
	}

	{
		FullTextIndex<FullTextRecord> fti("test_db2");

		struct SearchMetric metric;
		vector<FullTextRecord> result = SearchEngine::search(fti.shards(), {}, "josef", 1000, metric);
		BOOST_CHECK_EQUAL(result.size(), 1);
		BOOST_CHECK_EQUAL(result[0].m_value, URL("http://example.com").hash());

		result = SearchEngine::search(fti.shards(), {}, "åäö", 1000, metric);
		BOOST_CHECK_EQUAL(result.size(), 1);
		BOOST_CHECK_EQUAL(result[0].m_value, URL("http://example3.com").hash());

		result = SearchEngine::search(fti.shards(), {}, "jag", 1000, metric);
		BOOST_CHECK_EQUAL(result.size(), 2);
	}

	{
		FullTextIndexerRunner indexer("test_db3", "test_db3", "CC-MAIN-2021-17");
		indexer.truncate();
		indexer.index_text("http://example.com", "hej hopp josef", 1);
		indexer.index_text("http://example2.com", "hej jag heter test", 2);
		indexer.index_text("http://example3.com", "hej ett två åäö jag", 3);
		indexer.merge();
		indexer.sort();

		FullTextIndex<FullTextRecord> fti("test_db3");

		struct SearchMetric metric;
		vector<FullTextRecord> result = SearchEngine::search(fti.shards(), {}, "hej", 1000, metric);
		BOOST_CHECK_EQUAL(result.size(), 3);
		BOOST_CHECK_EQUAL(result[0].m_value, URL("http://example3.com").hash());
		BOOST_CHECK_EQUAL(result[1].m_value, URL("http://example2.com").hash());
		BOOST_CHECK_EQUAL(result[2].m_value, URL("http://example.com").hash());
	}

	{
		FullTextIndex<FullTextRecord> fti("test_db3");

		struct SearchMetric metric;
		vector<FullTextRecord> result = SearchEngine::search(fti.shards(), {}, "hej", 1000, metric);
		BOOST_CHECK_EQUAL(result.size(), 3);
		BOOST_CHECK_EQUAL(result[0].m_value, URL("http://example3.com").hash());
		BOOST_CHECK_EQUAL(result[1].m_value, URL("http://example2.com").hash());
		BOOST_CHECK_EQUAL(result[2].m_value, URL("http://example.com").hash());
	}

}

BOOST_AUTO_TEST_CASE(indexer_runner_2) {

	const string ft_db_name = "test_db_4";

	FullTextIndexerRunner indexer(ft_db_name, ft_db_name, "CC-MAIN-2021-17");
	indexer.truncate();

	ifstream infile("../tests/data/cc_index1");
	indexer.index_stream(infile);
	indexer.merge();
	indexer.sort();

	hash<string> hasher;

	vector<FullTextRecord> result;
	{
		FullTextIndex<FullTextRecord> fti(ft_db_name);

		struct SearchMetric metric;
		vector<FullTextRecord> result = SearchEngine::search(fti.shards(), {}, "Ariel Rockmore - ELA Study Skills - North Clayton Middle School",
			1000, metric);

		BOOST_CHECK(result.size() > 0 &&
			result[0].m_value == URL("http://017ccps.ss10.sharpschool.com/cms/One.aspx?portalId=64627&pageId=22360441").hash());

		result = SearchEngine::search(fti.shards(), {}, "Ariel Rockmore - ELA Study Skills - North Clayton Middle School", 1000, metric);

		BOOST_CHECK(result.size() > 0 &&
			result[0].m_value == URL("http://017ccps.ss10.sharpschool.com/cms/One.aspx?portalId=64627&pageId=22360441").hash());


		result = SearchEngine::search(fti.shards(), {}, "An Ode to Power", 1000, metric);
		BOOST_CHECK(result.size() > 0);

		bool contains_url = false;
		for (const FullTextRecord &res : result) {
			if (res.m_value == URL("http://aminorconsideration.org/apt-pupil-an-ode-to-power/").hash()) {
				contains_url = true;
				break;
			}
		}
		BOOST_CHECK(contains_url);

		result = SearchEngine::search(fti.shards(), {}, "Todos Fallado debatir pasado febrero", 1000, metric);
		BOOST_CHECK(result.size() > 0);

		contains_url = false;
		for (const FullTextRecord &res : result) {
			if (res.m_value == URL("http://artesacro.org/Noticia.asp?idreg=47157").hash()) {
				contains_url = true;
				break;
			}
		}
		BOOST_CHECK(contains_url);

	}
}

BOOST_AUTO_TEST_CASE(indexer) {

	{
		// Index full text
		HashTable hash_table("test_main_index");
		hash_table.truncate();

		HashTable hash_table_link("test_link_index");
		hash_table_link.truncate();

		SubSystem *sub_system = new SubSystem();
		for (size_t partition_num = 0; partition_num < 8; partition_num++) {
			FullTextIndexerRunner indexer("test_main_index_" + to_string(partition_num), "test_main_index", "ALEXANDRIA-TEST-01", sub_system);
			indexer.run(partition_num, 8);
		}
	}

	{
		// Index links
		UrlToDomain *url_to_domain = new UrlToDomain("main_index");
		url_to_domain->read();

		SubSystem *sub_system = new SubSystem();
		for (size_t partition_num = 0; partition_num < 8; partition_num++) {
			LinkIndexerRunner indexer("test_link_index_" + to_string(partition_num), "test_link_index", "ALEXANDRIA-TEST-01", sub_system,
				url_to_domain);
			indexer.run(partition_num, 8);
		}
	}

	{
		// Count elements in hash tables.
		HashTable hash_table("test_main_index");
		HashTable hash_table_link("test_link_index");

		BOOST_CHECK_EQUAL(hash_table.size(), 8);
		BOOST_CHECK_EQUAL(hash_table_link.size(), 11);

		// Make searches.
		vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("test_main_index", 8);
		vector<FullTextIndex<LinkFullTextRecord> *> link_index_array = FullText::create_index_array<LinkFullTextRecord>("test_link_index", 8);

		const string query = "Url1.com";
		struct SearchMetric metric;

		vector<LinkFullTextRecord> links = SearchEngine::search_link_array(link_index_array, query, 1000, metric);
		vector<FullTextRecord> results = SearchEngine::search_index_array(index_array, links, query, 1000, metric);

		BOOST_CHECK_EQUAL(links.size(), 1);
		BOOST_CHECK_EQUAL(results.size(), 1);
		BOOST_CHECK_EQUAL(metric.m_total_found, 1);
		BOOST_CHECK_EQUAL(metric.m_total_links_found, 1);
		BOOST_CHECK_EQUAL(metric.m_links_handled, 1);
		//BOOST_CHECK_EQUAL(metric.m_link_domain_matches, 1);
		BOOST_CHECK_EQUAL(metric.m_link_url_matches, 1);
		BOOST_CHECK_EQUAL(results[0].m_value, URL("http://url1.com/test").hash());

		FullText::delete_index_array<FullTextRecord>(index_array);
		FullText::delete_index_array<LinkFullTextRecord>(link_index_array);
	}

}

BOOST_AUTO_TEST_CASE(indexer_multiple_link_batches) {

	FullText::truncate_url_to_domain("main_index");
	FullText::truncate_index("test_link_index", 8);

	{
		// Index full text
		HashTable hash_table("test_main_index");
		hash_table.truncate();

		HashTable hash_table_link("test_link_index");
		hash_table_link.truncate();

		HashTable hash_table_domain_link("test_domain_link_index");
		hash_table_link.truncate();

		SubSystem *sub_system = new SubSystem();
		for (size_t partition_num = 0; partition_num < 8; partition_num++) {
			FullTextIndexerRunner indexer("test_main_index_" + to_string(partition_num), "test_main_index", "ALEXANDRIA-TEST-01", sub_system);
			indexer.run(partition_num, 8);
		}
	}

	{
		// Index links
		UrlToDomain *url_to_domain = new UrlToDomain("main_index");
		url_to_domain->read();

		BOOST_CHECK_EQUAL(url_to_domain->size(), 8);

		SubSystem *sub_system = new SubSystem();
		for (size_t partition_num = 0; partition_num < 8; partition_num++) {
			LinkIndexerRunnerNew indexer("test_link_index_" + to_string(partition_num), "test_domain_link_index_" + to_string(partition_num),
				"test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-01", sub_system, url_to_domain);
			indexer.run(partition_num, 8);
		}

	}

	{
		// Index more links and see if they get added with deduplication.
		UrlToDomain *url_to_domain = new UrlToDomain("main_index");
		url_to_domain->read();

		BOOST_CHECK_EQUAL(url_to_domain->size(), 8);

		SubSystem *sub_system = new SubSystem();
		for (size_t partition_num = 0; partition_num < 8; partition_num++) {
			LinkIndexerRunnerNew indexer("test_link_index_" + to_string(partition_num), "test_domain_link_index_" + to_string(partition_num),
				"test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-02", sub_system, url_to_domain);
			indexer.run(partition_num, 8);
		}

	}

	{
		// Count elements in hash tables.
		HashTable hash_table("test_main_index");
		HashTable hash_table_link("test_link_index");

		BOOST_CHECK_EQUAL(hash_table.size(), 8);
		BOOST_CHECK_EQUAL(hash_table_link.size(), 15);

		// Make searches.
		vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("test_main_index", 8);
		vector<FullTextIndex<LinkFullTextRecord> *> link_index_array = FullText::create_index_array<LinkFullTextRecord>("test_link_index", 8);

		const string query = "Url8.com";
		struct SearchMetric metric;

		{
			vector<LinkFullTextRecord> links = SearchEngine::search_link_array(link_index_array, query, 1000, metric);
			vector<FullTextRecord> results = SearchEngine::search_index_array(index_array, links, query, 1000, metric);

			BOOST_CHECK_EQUAL(links.size(), 3);
			BOOST_CHECK_EQUAL(results.size(), 1);
			BOOST_CHECK_EQUAL(metric.m_total_found, 1);
			BOOST_CHECK_EQUAL(metric.m_total_links_found, 3);
			BOOST_CHECK_EQUAL(metric.m_links_handled, 3);
			//BOOST_CHECK_EQUAL(metric.m_link_domain_matches, 2);
			BOOST_CHECK_EQUAL(metric.m_link_url_matches, 2);
			BOOST_CHECK_EQUAL(results[0].m_value, URL("http://url8.com/test").hash());
		}

		{
			vector<LinkFullTextRecord> links = SearchEngine::search_link_array(link_index_array, query, 1, metric);

			BOOST_CHECK_EQUAL(links.size(), 1);
			BOOST_CHECK_EQUAL(links[0].m_value, URL("http://url8.com/").link_hash(URL("http://url7.com/test"), "Link to url7.com from url8.com"));
		}

		FullText::delete_index_array<FullTextRecord>(index_array);
		FullText::delete_index_array<LinkFullTextRecord>(link_index_array);
	}

}

BOOST_AUTO_TEST_CASE(domain_links) {

	FullText::truncate_url_to_domain("main_index");
	FullText::truncate_index("test_link_index", 8);
	FullText::truncate_index("test_domain_link_index", 8);

	{
		// Index full text
		HashTable hash_table("test_main_index");
		hash_table.truncate();

		HashTable hash_table_link("test_link_index");
		hash_table_link.truncate();

		HashTable hash_table_domain_link("test_domain_link_index");
		hash_table_domain_link.truncate();

		SubSystem *sub_system = new SubSystem();
		for (size_t partition_num = 0; partition_num < 8; partition_num++) {
			FullTextIndexerRunner indexer("test_main_index_" + to_string(partition_num), "test_main_index", "ALEXANDRIA-TEST-05", sub_system);
			indexer.run(partition_num, 8);
		}
	}

	{
		// Index links
		UrlToDomain *url_to_domain = new UrlToDomain("main_index");
		url_to_domain->read();

		BOOST_CHECK_EQUAL(url_to_domain->size(), 9);

		SubSystem *sub_system = new SubSystem();
		for (size_t partition_num = 0; partition_num < 8; partition_num++) {
			LinkIndexerRunnerNew indexer("test_link_index_" + to_string(partition_num), "test_domain_link_index_" + to_string(partition_num),
				"test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-05", sub_system, url_to_domain);
			indexer.run(partition_num, 8);
		}

	}

	{
		// Count elements in hash tables.
		HashTable hash_table("test_main_index");
		HashTable hash_table_link("test_link_index");
		HashTable hash_table_domain_link("test_domain_link_index");

		BOOST_CHECK_EQUAL(hash_table.size(), 9);
		BOOST_CHECK_EQUAL(hash_table_link.size(), 12);
		BOOST_CHECK_EQUAL(hash_table_domain_link.size(), 11);

		// Make searches.
		vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("test_main_index", 8);
		vector<FullTextIndex<LinkFullTextRecord> *> link_index_array = FullText::create_index_array<LinkFullTextRecord>("test_link_index", 8);
		vector<FullTextIndex<DomainLinkFullTextRecord> *> domain_link_index_array =
			FullText::create_index_array<DomainLinkFullTextRecord>("test_domain_link_index", 8);

		const string query = "Testing the test from test04.links.gz";
		struct SearchMetric metric;

		{
			vector<DomainLinkFullTextRecord> links = SearchEngine::search_domain_link_array(domain_link_index_array, query, 1000, metric);

			BOOST_CHECK_EQUAL(links.size(), 1);
			BOOST_CHECK_EQUAL(metric.m_total_links_found, 1);
			BOOST_CHECK_EQUAL(links[0].m_value, URL("http://linksource4.com/").domain_link_hash(URL("http://url5.com/test"),
				"Testing the test from test04.links.gz"));
		}

		FullText::delete_index_array<FullTextRecord>(index_array);
		FullText::delete_index_array<LinkFullTextRecord>(link_index_array);
		FullText::delete_index_array<DomainLinkFullTextRecord>(domain_link_index_array);
	}

}

BOOST_AUTO_TEST_CASE(indexer_test_deduplication) {

	FullText::truncate_url_to_domain("main_index");

	{
		// Index full text
		HashTable hash_table("test_main_index");
		hash_table.truncate();

		SubSystem *sub_system = new SubSystem();
		for (size_t partition_num = 0; partition_num < 8; partition_num++) {
			FullTextIndexerRunner indexer("test_main_index_" + to_string(partition_num), "test_main_index", "ALEXANDRIA-TEST-03", sub_system);
			indexer.run(partition_num, 8);
		}
	}

	{
		// Count elements in hash tables.
		HashTable hash_table("test_main_index");

		BOOST_CHECK_EQUAL(hash_table.size(), 8);

		// Make searches.
		vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("test_main_index", 8);

		const string query = "my first url";
		struct SearchMetric metric;

		vector<FullTextRecord> results = SearchEngine::search_index_array(index_array, {}, query, 1000, metric);

		BOOST_CHECK_EQUAL(results.size(), 1);
		BOOST_CHECK_EQUAL(metric.m_total_found, 1);
		BOOST_CHECK_EQUAL(metric.m_total_links_found, 0);
		BOOST_CHECK_EQUAL(metric.m_links_handled, 0);
		BOOST_CHECK_EQUAL(metric.m_link_domain_matches, 0);
		BOOST_CHECK_EQUAL(metric.m_link_url_matches, 0);
		BOOST_CHECK_EQUAL(results[0].m_value, URL("http://url1.com/test").hash());

		results = SearchEngine::search_index_array(index_array, {}, "my second url", 1000, metric);
		BOOST_CHECK_EQUAL(results.size(), 1);
		BOOST_CHECK_EQUAL(metric.m_total_found, 1);
		BOOST_CHECK_EQUAL(metric.m_total_links_found, 0);
		BOOST_CHECK_EQUAL(metric.m_links_handled, 0);
		BOOST_CHECK_EQUAL(metric.m_link_domain_matches, 0);
		BOOST_CHECK_EQUAL(metric.m_link_url_matches, 0);
		BOOST_CHECK_EQUAL(results[0].m_value, URL("http://url2.com/test").hash());

		FullText::delete_index_array<FullTextRecord>(index_array);
	}

}

BOOST_AUTO_TEST_SUITE_END();
