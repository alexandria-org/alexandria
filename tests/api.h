
#include "api/Api.h"

BOOST_AUTO_TEST_SUITE(api)

BOOST_AUTO_TEST_CASE(api_search) {

	FullText::truncate_url_to_domain("main_index");
	FullText::truncate_index("test_link_index", 8);

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

		BOOST_CHECK_EQUAL(url_to_domain->size(), 8);

		SubSystem *sub_system = new SubSystem();
		for (size_t partition_num = 0; partition_num < 8; partition_num++) {
			LinkIndexerRunner indexer("test_link_index_" + to_string(partition_num), "test_link_index", "ALEXANDRIA-TEST-01", sub_system,
				url_to_domain);
			indexer.run(partition_num, 8);
		}

	}

	HashTable hash_table("test_main_index");
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
		Api::word_stats("Meta Description Text", index_array, link_index_array, response_stream);

		string response = response_stream.str();

		Aws::Utils::Json::JsonValue json(response);

		auto v = json.View();

		BOOST_CHECK(v.ValueExists("status"));
		BOOST_CHECK_EQUAL(v.GetString("status"), "success");

		BOOST_CHECK(v.ValueExists("index"));
		BOOST_CHECK(v.GetObject("index").ValueExists("words"));

		BOOST_CHECK(v.ValueExists("link_index"));
		BOOST_CHECK(v.GetObject("link_index").ValueExists("words"));

	}

	FullText::delete_index_array<FullTextRecord>(index_array);
	FullText::delete_index_array<LinkFullTextRecord>(link_index_array);

}

BOOST_AUTO_TEST_SUITE_END();
