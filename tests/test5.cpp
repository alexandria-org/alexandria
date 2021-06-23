

#include "test5.h"
#include "full_text/FullTextIndex.h"
#include "full_text/FullTextIndexerRunner.h"

using namespace std;

/*
 * Test Full Text Index
 */
int test5_1(void) {
	int ok = 1;

	std::hash<string> hasher;

	{

		FullTextIndexerRunner indexer("test_db1", "CC-MAIN-2021-17");
		indexer.truncate();
		indexer.index_text("http://aciedd.org/fixing-solar-panels/	Fixing Solar Panels ‚Äì blog	blog		Menu Home Search for: Posted in General Fixing Solar Panels Author: Holly Montgomery Published Date: December 24, 2020 Leave a Comment on Fixing Solar Panels Complement your renewable power project with Perfection fashionable solar panel assistance structures. If you live in an region that receives a lot of snow in the winter, becoming able to easily sweep the snow off of your solar panels is a major comfort. If your solar panel contractor advises you that horizontal solar panels are the greatest selection for your solar wants, you do not need to have a particular inverter. The Solar PV panels are then clamped to the rails, keeping the panels really close to the roof to decrease wind loading. For 1 point, solar panels require to face either south or west to get direct sunlight. Once you have bought your solar panel you will need to have to determine on a safe fixing method, our extensive variety of permanent and non permane");
		indexer.merge();

		FullTextIndex fti("test_db1");
		vector<FullTextResult> result = fti.search_word("permanent");
		ok = ok && result.size() == 1;
		ok = ok && result[0].m_value == hasher("http://aciedd.org/fixing-solar-panels/");
	}

	{

		FullTextIndexerRunner indexer("test_db2", "CC-MAIN-2021-17");
		indexer.truncate();
		indexer.index_text("http://example.com	title	h1	meta	Hej hopp josef");
		indexer.index_text("http://example2.com	title	h1	meta	jag heter test");
		indexer.index_text("http://example3.com	title	h1	meta	ett två åäö jag testar");
		indexer.merge();

		FullTextIndex fti("test_db2");

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
		FullTextIndex fti("test_db2");

		vector<FullTextResult> result = fti.search_word("josef");
		ok = ok && result.size() == 1;
		ok = ok && result[0].m_value == hasher("http://example.com");

		result = fti.search_word("åäö");
		ok = ok && result.size() == 1;
		ok = ok && result[0].m_value == hasher("http://example3.com");

		result = fti.search_word("jag");
		ok = ok && result.size() == 2;
	}

	{
		FullTextIndexerRunner indexer("test_db3", "CC-MAIN-2021-17");
		indexer.truncate();
		indexer.index_text("http://example.com", "hej hopp josef", 1);
		indexer.index_text("http://example2.com", "hej jag heter test", 2);
		indexer.index_text("http://example3.com", "hej ett två åäö jag", 3);
		indexer.merge();

		FullTextIndex fti("test_db3");

		vector<FullTextResult> result = fti.search_word("hej");
		ok = ok && result.size() == 3;
		ok = ok && result[0].m_value == hasher("http://example3.com");
		ok = ok && result[1].m_value == hasher("http://example2.com");
		ok = ok && result[2].m_value == hasher("http://example.com");
	}

	{
		FullTextIndex fti("test_db3");

		vector<FullTextResult> result = fti.search_word("hej");
		ok = ok && result.size() == 3;
		ok = ok && result[0].m_value == hasher("http://example3.com");
		ok = ok && result[1].m_value == hasher("http://example2.com");
		ok = ok && result[2].m_value == hasher("http://example.com");
	}

	return ok;
}

int test5_2(void) {
	int ok = 1;

	std::hash<string> hasher;

	/*

	vector<FullTextResult> result;
	{
		FullTextIndex fti("test_db_4");

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

	/*map<size_t, vector<uint64_t>> values = {
		{0, {1, 2, 3}},
		{1, {3, 4, 5}},
		{2, {2, 3, 6}}	
	};

	{
		FullTextIndex fti("test_db_5");

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
	}*/

	return ok;
}
