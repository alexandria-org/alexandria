

#include "test5.h"
#include "full_text/FullTextIndex.h"

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

using namespace std;
using namespace boost::iostreams;

/*
 * Test Full Text Index
 */
int test5_1(void) {
	int ok = 1;

	std::hash<string> hasher;

	cout << hasher("hej") << endl;
	cout << hasher("hopp") << endl;
	cout << hasher("josef") << endl;

	FullTextIndex fti("test_db1");
	if (fti.disk_size() == 0) {
		fti.add("http://example.com", "Hej hopp josef");
	}
	vector<FullTextResult> result = fti.search_word("hopp");
	ok = ok && result.size() == 1;
	ok = ok && result[0].m_value == hasher("http://example.com");

	fti.save();

	FullTextIndex fti2("test_db_2");
	fti2.add("http://example.com", "Hej hopp josef");
	fti2.add("http://example2.com", "jag heter test");
	fti2.add("http://example3.com", "ett två åäö jag");
	fti2.save();

	result = fti2.search_word("josef");
	ok = ok && result.size() == 1;
	ok = ok && result[0].m_value == hasher("http://example.com");

	result = fti2.search_word("åäö");
	ok = ok && result.size() == 1;
	ok = ok && result[0].m_value == hasher("http://example3.com");

	result = fti2.search_word("jag");
	ok = ok && result.size() == 2;
	//ok = ok && result[0].m_value == hasher("http://example.com");

	FullTextIndex fti3("test_db_2");

	result = fti3.search_word("josef");
	ok = ok && result.size() == 1;
	ok = ok && result[0].m_value == hasher("http://example.com");

	result = fti3.search_word("åäö");
	ok = ok && result.size() == 1;
	ok = ok && result[0].m_value == hasher("http://example3.com");

	result = fti3.search_word("jag");
	ok = ok && result.size() == 2;

	fti3.truncate();

	FullTextIndex fti4("test_db_3");
	fti4.add("http://example.com", "hej hopp josef", 1);
	fti4.add("http://example2.com", "hej jag heter test", 2);
	fti4.add("http://example3.com", "hej ett två åäö jag", 3);
	fti4.save();

	result = fti4.search_word("hej");
	ok = ok && result.size() == 3;
	ok = ok && result[0].m_value == hasher("http://example3.com");
	ok = ok && result[1].m_value == hasher("http://example2.com");
	ok = ok && result[2].m_value == hasher("http://example.com");

	FullTextIndex fti5("test_db_3");

	result = fti5.search_word("hej");
	ok = ok && result.size() == 3;
	ok = ok && result[0].m_value == hasher("http://example3.com");
	ok = ok && result[1].m_value == hasher("http://example2.com");
	ok = ok && result[2].m_value == hasher("http://example.com");
	fti5.truncate();

	return ok;
}

int test5_2(void) {
	int ok = 1;

	std::hash<string> hasher;

	ifstream file("../tests/data/cc_index1.gz");
	filtering_istream decompress_stream;
	decompress_stream.push(gzip_decompressor());
	decompress_stream.push(file);

	vector<FullTextResult> result;
	FullTextIndex index("test_db_4");
	Profiler profile("add_stream");
	index.add_stream(decompress_stream, {1, 2, 3, 4}, {1, 1, 1, 1});
	profile.stop();

	result = index.search_phrase("Ariel Rockmore - ELA Study Skills - North Clayton Middle School");

	ok = ok && result.size() > 0 &&
		result[0].m_value == hasher("http://017ccps.ss10.sharpschool.com/cms/One.aspx?portalId=64627&pageId=22360441");

	index.save();

	Profiler profile_search_db("search_db");
	result = index.search_phrase("Ariel Rockmore - ELA Study Skills - North Clayton Middle School");
	profile_search_db.stop();

	for (FullTextResult &res : result) {
		cout << "Result: " << res.m_value << endl;
	}
	cout << "hash: " << hasher("http://017ccps.ss10.sharpschool.com/cms/One.aspx?portalId=64627&pageId=22360441") << endl;

	ok = ok && result.size() > 0 &&
		result[0].m_value == hasher("http://017ccps.ss10.sharpschool.com/cms/One.aspx?portalId=64627&pageId=22360441");

	return ok;
}

int test5_3(void) {
	int ok = 1;

	map<size_t, vector<uint64_t>> values = {
		{0, {1, 2, 3}},
		{1, {3, 4, 5}},
		{2, {2, 3, 6}}	
	};

	FullTextIndex index("test_db_5");

	size_t shortest_vector;
	vector<size_t> result = index.value_intersection(values, shortest_vector);
	ok = ok && result.size() == 1 && values[shortest_vector][result[0]] == 3;

	values = {
		{0, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}},
		{1, {3, 4, 5, 6, 7}},
		{2, {2, 3, 6, 7, 8, 9}}	
	};

	result = index.value_intersection(values, shortest_vector);
	ok = ok && shortest_vector == 1;
	ok = ok && result.size() == 3;
	ok = ok && values[shortest_vector][result[0]] == 3;
	ok = ok && values[shortest_vector][result[1]] == 6;
	ok = ok && values[shortest_vector][result[2]] == 7;

	return ok;
}