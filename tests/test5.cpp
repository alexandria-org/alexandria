

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

	FullTextIndex fti("test_db1");
	if (fti.size() == 0) {
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
	index.add_stream(decompress_stream, {1}, {1});
	profile.stop();

	result = index.search_phrase("Ariel Rockmore - ELA Study Skills - North Clayton Middle School");

	for (FullTextResult &res : result) {
		cout << "Result: " << res.m_value << endl;
	}

	ok = ok && result.size() > 0 && result[0].m_value == hasher("http://017ccps.ss10.sharpschool.com/cms/One.aspx?portalId=64627&pageId=22360441");

	return ok;
}
