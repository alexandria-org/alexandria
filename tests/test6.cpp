
#include "test6.h"
#include "hash_table/HashTable.h"
#include "hash_table/HashTableShardBuilder.h"
#include "full_text/FullTextIndex.h"
#include "full_text/FullTextIndexer.h"
#include "full_text/FullTextIndexerRunner.h"
#include "link_index/LinkIndexerRunner.h"

using namespace std;

/*
 * Test Hash Table
 */
int test6_1(void) {
	int ok = 1;

	{
		HashTable hash_table("test_index");
		hash_table.truncate();
		hash_table.add(123, "hejsan");
	}

	{
		HashTable hash_table("test_index");
		ok = ok && hash_table.find(123) == "hejsan";
		ok = ok && hash_table.find(1234) == "";
	}

	{
		HashTable hash_table("test_index");
		hash_table.truncate();
		hash_table.add(123, "testing");
	}

	{
		HashTable hash_table("test_index");
		ok = ok && hash_table.find(123) == "testing";
		ok = ok && hash_table.find(123) == "testing";
	}

	{
		HashTable hash_table("test_index");
		hash_table.truncate();

		for (size_t i = 20000; i < 20010; i++) {
			hash_table.add(i, "testing" + to_string(i));
		}

		for (size_t i = 1; i < 20000; i++) {
			hash_table.add(i, "testing" + to_string(i));
		}

		for (size_t i = 30000; i < 40000; i++) {
			hash_table.add(i, "testing" + to_string(i));
		}

		hash_table.sort();
	}

	{
		HashTable hash_table("test_index");
		for (size_t i = 20000; i < 20010; i++) {
			ok = ok && (hash_table.find(i) == "testing" + to_string(i));
		}
	}

	return ok;
}

int test6_2(void) {
	int ok = 1;

	/* Test to add and retrieve large objects. */

	{
		HashTable hash_table("test_index");
		hash_table.truncate();

		string long_data(10000, 'a');
		hash_table.add(1337, long_data);
	}

	{
		HashTable hash_table("test_index");
		ok = ok && hash_table.find(1337) == string(10000, 'a');
		ok = ok && hash_table.find(1337) == string(10000, 'a');
		ok = ok && hash_table.find(1337) == string(10000, 'a');
	}

	return ok;
}

int test6_3(void) {
	int ok = 1;

	const string ft_db_name = "test_db1";
	const string li_db_name = "test_li1";

	FullTextIndexerRunner indexer(ft_db_name, "CC-MAIN-2021-17");
	indexer.truncate();

	ifstream infile("../tests/data/test6_2_ft.txt");
	indexer.index_stream(infile);
	indexer.merge();
	indexer.sort();

	{
		HashTable hash_table(ft_db_name);
		FullTextIndex fti(ft_db_name);

		size_t total;
		vector<FullTextResult> result = fti.search_phrase("quick brown fox", 1000, total);

		ok = ok && total == 1;
		ok = ok && result.size() == 1;
		ok = ok && result[0].m_score == 0;
		ok = ok && (hash_table.find(result[0].m_value) == "http://url1.com");
	}

	// Add links
	LinkIndexerRunner link_indexer(li_db_name, "CC-MAIN-2021-17", ft_db_name);
	link_indexer.truncate();

	ifstream infile2("../tests/data/test6_2_li.txt");
	link_indexer.index_stream(infile2);
	link_indexer.merge();
	link_indexer.sort();

	{
		HashTable hash_table(ft_db_name);
		FullTextIndex fti(ft_db_name);

		{
			size_t total;
			vector<FullTextResult> result = fti.search_phrase("fox", 1000, total);

			ok = ok && total == 1;
			ok = ok && result.size() == 1;
			ok = ok && result[0].m_score == 1000;
			ok = ok && (hash_table.find(result[0].m_value) == "http://url1.com");
		}

		{
			size_t total;
			vector<FullTextResult> result = fti.search_phrase("josef", 1000, total);

			ok = ok && total == 1;
			ok = ok && result.size() == 1;
			ok = ok && result[0].m_score == 2000;
			ok = ok && (hash_table.find(result[0].m_value) == "http://url2.com/sub_page");
		}
	}

	return ok;
}

int test6_4(void) {
	int ok = 1;

	const string ft_db_name = "test_db1";
	const string li_db_name = "test_li1";

	FullTextIndexerRunner indexer(ft_db_name, "CC-MAIN-2021-17");
	indexer.truncate();

	ifstream infile("../tests/data/test6_2_ft.txt");
	indexer.index_stream(infile);
	indexer.merge();
	indexer.sort();

	{
		HashTable hash_table(ft_db_name);
		FullTextIndex fti(ft_db_name);

		size_t total;
		vector<FullTextResult> result = fti.search_phrase("quick brown fox", 1000, total);

		ok = ok && total == 1;
		ok = ok && result.size() == 1;
		ok = ok && result[0].m_score == 0;
		ok = ok && (hash_table.find(result[0].m_value) == "http://url1.com");
	}

	// Add links
	LinkIndexerRunner link_indexer(li_db_name, "CC-MAIN-2021-17", ft_db_name);
	link_indexer.truncate();

	ifstream infile2("../tests/data/test6_2_li.txt");
	link_indexer.index_stream(infile2);
	link_indexer.merge();
	link_indexer.sort();

	{
		HashTable hash_table(ft_db_name);
		FullTextIndex fti(ft_db_name);

		{
			size_t total;
			vector<FullTextResult> result = fti.search_phrase("quick brown fox", 1000, total);

			ok = ok && total == 1;
			ok = ok && result.size() == 1;
			ok = ok && result[0].m_score == 1000;
			ok = ok && (hash_table.find(result[0].m_value) == "http://url1.com");
		}

		{
			size_t total;
			vector<FullTextResult> result = fti.search_phrase("josef and jesus", 1000, total);

			ok = ok && total == 1;
			ok = ok && result.size() == 1;
			ok = ok && result[0].m_score == 1000;
			ok = ok && (hash_table.find(result[0].m_value) == "http://url2.com/sub_page");
		}
	}

	return ok;
}

