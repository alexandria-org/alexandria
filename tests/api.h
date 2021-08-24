
#include "hash_table/HashTableHelper.h"
#include "api/Api.h"

BOOST_AUTO_TEST_SUITE(api)

BOOST_AUTO_TEST_CASE(api_search) {

	FullText::truncate_url_to_domain("main_index");
	FullText::truncate_index("test_link_index", 8);

	HashTableHelper::truncate("test_main_index");
	HashTableHelper::truncate("test_link_index");
	HashTableHelper::truncate("test_domain_link_index");

	{
		// Index full text
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
			LinkIndexerRunner indexer("test_link_index_" + to_string(partition_num), "test_domain_link_index_" + to_string(partition_num),
				"test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-01", sub_system, url_to_domain);
			indexer.run(partition_num, 8);
		}
	}

	HashTable hash_table("test_main_index");
	HashTable link_hash_table("test_link_index");
	vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("test_main_index", 8);
	vector<FullTextIndex<LinkFullTextRecord> *> link_index_array = FullText::create_index_array<LinkFullTextRecord>("test_link_index", 8);

	{
		stringstream response_stream;
		Api::search("url1.com", hash_table, index_array, link_index_array, response_stream);

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
	FullText::truncate_index("test_link_index", 8);

	HashTableHelper::truncate("test_main_index");
	HashTableHelper::truncate("test_link_index");
	HashTableHelper::truncate("test_domain_link_index");

	{
		// Index full text
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
			LinkIndexerRunner indexer("test_link_index_" + to_string(partition_num), "test_domain_link_index_" + to_string(partition_num),
				"test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-01", sub_system, url_to_domain);
			indexer.run(partition_num, 8);
		}
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

		BOOST_CHECK(v.ValueExists("results"));
		BOOST_CHECK(v.GetArray("results")[0].ValueExists("url"));
		BOOST_CHECK_EQUAL(v.GetArray("results")[0].GetString("url"), "http://url1.com/test");
	}

	FullText::delete_index_array<FullTextRecord>(index_array);
	FullText::delete_index_array<LinkFullTextRecord>(link_index_array);
	FullText::delete_index_array<DomainLinkFullTextRecord>(domain_link_index_array);

}

BOOST_AUTO_TEST_CASE(api_word_stats) {

	FullText::truncate_url_to_domain("main_index");
	FullText::truncate_index("test_link_index", 8);

	HashTableHelper::truncate("test_main_index");
	HashTableHelper::truncate("test_link_index");
	HashTableHelper::truncate("test_domain_link_index");

	{
		// Index full text
		SubSystem *sub_system = new SubSystem();
		for (size_t partition_num = 0; partition_num < 8; partition_num++) {
			FullTextIndexerRunner indexer("test_main_index_" + to_string(partition_num), "test_main_index", "ALEXANDRIA-TEST-02", sub_system);
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
			LinkIndexerRunner indexer("test_link_index_" + to_string(partition_num), "test_domain_link_index_" + to_string(partition_num),
				"test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-02", sub_system,
				url_to_domain);
			indexer.run(partition_num, 8);
		}
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
	FullText::truncate_index("test_link_index", 8);

	HashTableHelper::truncate("test_main_index");
	HashTableHelper::truncate("test_link_index");
	HashTableHelper::truncate("test_domain_link_index");

	{
		// Index full text
		SubSystem *sub_system = new SubSystem();
		for (size_t partition_num = 0; partition_num < 8; partition_num++) {
			FullTextIndexerRunner indexer("test_main_index_" + to_string(partition_num), "test_main_index", "ALEXANDRIA-TEST-04", sub_system);
			indexer.run(partition_num, 8);
		}
	}

	{
		// Index links
		UrlToDomain *url_to_domain = new UrlToDomain("main_index");
		url_to_domain->read();

		SubSystem *sub_system = new SubSystem();
		for (size_t partition_num = 0; partition_num < 8; partition_num++) {
			LinkIndexerRunner indexer("test_link_index_" + to_string(partition_num), "test_domain_link_index_" + to_string(partition_num),
				"test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-04", sub_system,
				url_to_domain);
			indexer.run(partition_num, 8);
		}
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

		BOOST_CHECK_EQUAL(v.GetString("response"), "http://url1.com/my_test_url	test test		test test test test	");
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

BOOST_AUTO_TEST_CASE(api_links) {

	FullText::truncate_url_to_domain("main_index");
	FullText::truncate_index("test_link_index", 8);

	HashTableHelper::truncate("test_main_index");
	HashTableHelper::truncate("test_link_index");
	HashTableHelper::truncate("test_domain_link_index");

	{
		// Index full text
		SubSystem *sub_system = new SubSystem();
		for (size_t partition_num = 0; partition_num < 8; partition_num++) {
			FullTextIndexerRunner indexer("test_main_index_" + to_string(partition_num), "test_main_index", "ALEXANDRIA-TEST-04", sub_system);
			indexer.run(partition_num, 8);
		}
	}

	{
		// Index links
		UrlToDomain *url_to_domain = new UrlToDomain("main_index");
		url_to_domain->read();

		SubSystem *sub_system = new SubSystem();
		for (size_t partition_num = 0; partition_num < 8; partition_num++) {
			LinkIndexerRunner indexer("test_link_index_" + to_string(partition_num), "test_domain_link_index_" + to_string(partition_num),
				"test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-04", sub_system, url_to_domain);
			indexer.run(partition_num, 8);
		}
	}

	HashTable link_hash_table("test_link_index");
	vector<FullTextIndex<LinkFullTextRecord> *> link_index_array = FullText::create_index_array<LinkFullTextRecord>("test_link_index", 8);

	{
		stringstream response_stream;
		Api::search_links("star trek guinan", link_hash_table, link_index_array, response_stream);

		string response = response_stream.str();

		Aws::Utils::Json::JsonValue json(response);

		auto v = json.View();

		BOOST_CHECK(v.ValueExists("status"));

#ifdef COMPILE_WITH_LINK_INDEX
		BOOST_CHECK_EQUAL(v.GetString("status"), "success");

		BOOST_CHECK(v.ValueExists("results"));
		BOOST_CHECK(v.GetArray("results")[0].ValueExists("source_url"));
		BOOST_CHECK_EQUAL(v.GetArray("results").GetLength(), 1);
		BOOST_CHECK_EQUAL(v.GetArray("results")[0].GetString("source_url"), "http://214th.blogspot.com/2016/03/from-potemkin-pictures-battle-cruiser.html");
		BOOST_CHECK_EQUAL(v.GetArray("results")[0].GetString("target_url"), "http://url1.com/my_test_url");
		BOOST_CHECK_EQUAL(v.GetArray("results")[0].GetString("link_text"), "Star Trek Guinan");
#else
		BOOST_CHECK_EQUAL(v.GetString("status"), "error");
#endif
	}

	{
		stringstream response_stream;
		Api::search_links("non existing link", link_hash_table, link_index_array, response_stream);

		string response = response_stream.str();

		Aws::Utils::Json::JsonValue json(response);

		auto v = json.View();

		BOOST_CHECK(v.ValueExists("status"));
#ifdef COMPILE_WITH_LINK_INDEX
		BOOST_CHECK_EQUAL(v.GetString("status"), "success");

		BOOST_CHECK(v.ValueExists("results"));
		BOOST_CHECK_EQUAL(v.GetArray("results").GetLength(), 0);
#else
		BOOST_CHECK_EQUAL(v.GetString("status"), "error");
#endif
	}

	FullText::delete_index_array<LinkFullTextRecord>(link_index_array);
}

BOOST_AUTO_TEST_CASE(api_domain_links) {

	FullText::truncate_url_to_domain("main_index");
	FullText::truncate_index("test_link_index", 8);

	HashTableHelper::truncate("test_main_index");
	HashTableHelper::truncate("test_link_index");
	HashTableHelper::truncate("test_domain_link_index");

	{
		// Index full text
		SubSystem *sub_system = new SubSystem();
		for (size_t partition_num = 0; partition_num < 8; partition_num++) {
			FullTextIndexerRunner indexer("test_main_index_" + to_string(partition_num), "test_main_index", "ALEXANDRIA-TEST-04", sub_system);
			indexer.run(partition_num, 8);
		}
	}

	{
		// Index links
		UrlToDomain *url_to_domain = new UrlToDomain("main_index");
		url_to_domain->read();

		SubSystem *sub_system = new SubSystem();
		for (size_t partition_num = 0; partition_num < 8; partition_num++) {
			LinkIndexerRunner indexer("test_link_index_" + to_string(partition_num), "test_domain_link_index_" + to_string(partition_num),
				"test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-04", sub_system, url_to_domain);
			indexer.run(partition_num, 8);
		}
	}

	HashTable domain_link_hash_table("test_link_index");
	vector<FullTextIndex<DomainLinkFullTextRecord> *> domain_link_index_array =
		FullText::create_index_array<DomainLinkFullTextRecord>("test_domain_link_index", 8);

	{
		stringstream response_stream;
		Api::search_domain_links("star trek guinan", domain_link_hash_table, domain_link_index_array, response_stream);

		string response = response_stream.str();

		Aws::Utils::Json::JsonValue json(response);

		auto v = json.View();

		BOOST_CHECK(v.ValueExists("status"));

#ifdef COMPILE_WITH_LINK_INDEX
		BOOST_CHECK_EQUAL(v.GetString("status"), "success");

		BOOST_CHECK(v.ValueExists("results"));
		BOOST_CHECK(v.GetArray("results")[0].ValueExists("source_url"));
		BOOST_CHECK_EQUAL(v.GetArray("results").GetLength(), 1);
#else
		BOOST_CHECK_EQUAL(v.GetString("status"), "error");
#endif
	}

	{
		stringstream response_stream;
		Api::search_domain_links("non existing link", domain_link_hash_table, domain_link_index_array, response_stream);

		string response = response_stream.str();

		Aws::Utils::Json::JsonValue json(response);

		auto v = json.View();

		BOOST_CHECK(v.ValueExists("status"));

#ifdef COMPILE_WITH_LINK_INDEX
		BOOST_CHECK_EQUAL(v.GetString("status"), "success");

		BOOST_CHECK(v.ValueExists("results"));
		BOOST_CHECK_EQUAL(v.GetArray("results").GetLength(), 0);
#else
		BOOST_CHECK_EQUAL(v.GetString("status"), "error");
#endif
	}

	FullText::delete_index_array<DomainLinkFullTextRecord>(domain_link_index_array);
}

BOOST_AUTO_TEST_CASE(api_search_deduplication) {

	FullText::truncate_url_to_domain("main_index");
	FullText::truncate_index("test_link_index", 8);

	HashTableHelper::truncate("test_main_index");
	HashTableHelper::truncate("test_link_index");
	HashTableHelper::truncate("test_domain_link_index");

	Config::nodes_in_cluster = 2;
	Config::node_id = 0;

	// url8.com should be in node 0
	// url1-7.com should be in node 1

	{
		// Index full text
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

		BOOST_CHECK_EQUAL(url_to_domain->size(), 1);

		SubSystem *sub_system = new SubSystem();
		for (size_t partition_num = 0; partition_num < 8; partition_num++) {
			LinkIndexerRunner indexer("test_link_index_" + to_string(partition_num), "test_domain_link_index_" + to_string(partition_num),
				"test_link_index", "test_domain_link_index", "ALEXANDRIA-TEST-01", sub_system, url_to_domain);
			indexer.run(partition_num, 8);
		}
	}

	HashTable hash_table("test_main_index");
	HashTable link_hash_table("test_link_index");
	vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("test_main_index", 8);
	vector<FullTextIndex<LinkFullTextRecord> *> link_index_array = FullText::create_index_array<LinkFullTextRecord>("test_link_index", 8);

	{
		stringstream response_stream;
		Api::search("url1.com", hash_table, index_array, link_index_array, response_stream);

		string response = response_stream.str();

		Aws::Utils::Json::JsonValue json(response);

		auto v = json.View();

		BOOST_CHECK(v.ValueExists("status"));
		BOOST_CHECK_EQUAL(v.GetString("status"), "success");

		BOOST_CHECK(v.ValueExists("results"));
		BOOST_CHECK_EQUAL(v.GetArray("results").GetLength(), 0);
	}

	// Reset config.

	Config::nodes_in_cluster = 1;
	Config::node_id = 0;

}

BOOST_AUTO_TEST_SUITE_END();
