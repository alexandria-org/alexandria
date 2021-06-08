

#include "test5.h"
#include "full_text/FullTextIndex.h"

using namespace std;

/*
 * Test Full Text Index
 */
int test5_1(void) {
	int ok = 1;

	/*
	std::hash<string> hasher;

	{
		// Build shard.
		FullTextShardBuilder builder("/mnt/tmp_builder");
		builder.add(hasher())

		FullTextIndex fti("test_db1");
		fti.wait_for_start();
		if (fti.disk_size() == 0) {
			fti.add("http://example.com", "Hej hopp josef");
		}
		vector<FullTextResult> result = fti.search_word("hopp");
		ok = ok && result.size() == 1;
		ok = ok && result[0].m_value == hasher("http://example.com");

		fti.save();
	}

	{
		FullTextIndex fti("test_db_2");
		fti.wait_for_start();
		fti.add("http://example.com", "Hej hopp josef");
		fti.add("http://example2.com", "jag heter test");
		fti.add("http://example3.com", "ett två åäö jag testar");
		fti.save();

		vector<FullTextResult> result = fti.search_word("josef");
		ok = ok && result.size() == 1;
		ok = ok && result[0].m_value == hasher("http://example.com");

		result = fti.search_word("åäö");
		ok = ok && result.size() == 1;
		ok = ok && result[0].m_value == hasher("http://example3.com");

		result = fti.search_word("testar");
		ok = ok && result.size() == 1;
		ok = ok && result[0].m_value == hasher("http://example3.com");
	}

	{
		FullTextIndex fti("test_db_2");
		fti.wait_for_start();

		vector<FullTextResult> result = fti.search_word("josef");
		ok = ok && result.size() == 1;
		ok = ok && result[0].m_value == hasher("http://example.com");

		result = fti.search_word("åäö");
		ok = ok && result.size() == 1;
		ok = ok && result[0].m_value == hasher("http://example3.com");

		result = fti.search_word("jag");
		ok = ok && result.size() == 2;

		fti.truncate();
	}

	{
		FullTextIndex fti("test_db_3");
		fti.wait_for_start();

		fti.add("http://example.com", "hej hopp josef", 1);
		fti.add("http://example2.com", "hej jag heter test", 2);
		fti.add("http://example3.com", "hej ett två åäö jag", 3);
		fti.save();

		vector<FullTextResult> result = fti.search_word("hej");
		ok = ok && result.size() == 3;
		ok = ok && result[0].m_value == hasher("http://example3.com");
		ok = ok && result[1].m_value == hasher("http://example2.com");
		ok = ok && result[2].m_value == hasher("http://example.com");
	}

	{
		FullTextIndex fti("test_db_3");
		fti.wait_for_start();

		vector<FullTextResult> result = fti.search_word("hej");
		ok = ok && result.size() == 3;
		ok = ok && result[0].m_value == hasher("http://example3.com");
		ok = ok && result[1].m_value == hasher("http://example2.com");
		ok = ok && result[2].m_value == hasher("http://example.com");
		fti.truncate();
	}
	*/

	return ok;
}

int test5_2(void) {
	int ok = 1;

	std::hash<string> hasher;

	/*

	vector<FullTextResult> result;
	{
		FullTextIndex fti("test_db_4");
		fti.wait_for_start();

		Profiler profile("add_file");
		fti.add_file("../tests/data/cc_index1.gz", {1, 2, 3, 4}, {1, 1, 1, 1});
		profile.stop();

		result = fti.search_phrase("Ariel Rockmore - ELA Study Skills - North Clayton Middle School");

		ok = ok && result.size() > 0 &&
			result[0].m_value == hasher("http://017ccps.ss10.sharpschool.com/cms/One.aspx?portalId=64627&pageId=22360441");

		fti.save();

		Profiler profile_search_db("search_db");
		result = fti.search_phrase("Ariel Rockmore - ELA Study Skills - North Clayton Middle School");
		profile_search_db.stop();

		ok = ok && result.size() > 0 &&
			result[0].m_value == hasher("http://017ccps.ss10.sharpschool.com/cms/One.aspx?portalId=64627&pageId=22360441");
	}
	*/

	return ok;
}

int test5_3(void) {
	int ok = 1;

	map<size_t, vector<uint64_t>> values = {
		{0, {1, 2, 3}},
		{1, {3, 4, 5}},
		{2, {2, 3, 6}}	
	};

	{
		FullTextIndex fti("test_db_5");
		fti.wait_for_start();

		size_t shortest_vector;
		vector<size_t> result = fti.value_intersection(values, shortest_vector);
		ok = ok && result.size() == 1 && values[shortest_vector][result[0]] == 3;

		values = {
			{0, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}},
			{1, {3, 4, 5, 6, 7}},
			{2, {2, 3, 6, 7, 8, 9}}	
		};

		result = fti.value_intersection(values, shortest_vector);
		ok = ok && shortest_vector == 1;
		ok = ok && result.size() == 3;
		ok = ok && values[shortest_vector][result[0]] == 3;
		ok = ok && values[shortest_vector][result[1]] == 6;
		ok = ok && values[shortest_vector][result[2]] == 7;
	}

	return ok;
}
