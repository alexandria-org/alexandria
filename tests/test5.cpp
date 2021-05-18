

#include "test5.h"
#include "FullTextIndex.h"

using namespace std;

/*
 * Test Search API
 */
int test5_1(void) {
	int ok = 1;

	std::hash<string> hasher;

	FullTextIndex fti("test_db1");
	if (fti.size() == 0) {
		fti.add("http://example.com", "Hej hopp josef");
	}
	vector<FullTextResult> result = fti.search("hopp");
	ok = ok && result.size() == 1;
	ok = ok && result[0].m_value == hasher("http://example.com");

	fti.save();

	FullTextIndex fti2("test_db_2");
	fti2.add("http://example.com", "Hej hopp josef");
	fti2.add("http://example2.com", "jag heter test");
	fti2.add("http://example3.com", "ett två åäö jag");
	fti2.save();

	result = fti2.search("josef");
	ok = ok && result.size() == 1;
	ok = ok && result[0].m_value == hasher("http://example.com");

	result = fti2.search("åäö");
	ok = ok && result.size() == 1;
	ok = ok && result[0].m_value == hasher("http://example3.com");

	result = fti2.search("jag");
	ok = ok && result.size() == 2;
	//ok = ok && result[0].m_value == hasher("http://example.com");

	FullTextIndex fti3("test_db_2");

	result = fti3.search("josef");
	ok = ok && result.size() == 1;
	ok = ok && result[0].m_value == hasher("http://example.com");

	result = fti3.search("åäö");
	ok = ok && result.size() == 1;
	ok = ok && result[0].m_value == hasher("http://example3.com");

	result = fti3.search("jag");
	ok = ok && result.size() == 2;

	fti3.truncate();

	FullTextIndex fti4("test_db_3");
	fti4.add("http://example.com", "hej hopp josef", 1);
	fti4.add("http://example2.com", "hej jag heter test", 2);
	fti4.add("http://example3.com", "hej ett två åäö jag", 3);
	fti4.save();

	result = fti4.search("hej");
	ok = ok && result.size() == 3;
	ok = ok && result[0].m_value == hasher("http://example3.com");
	ok = ok && result[1].m_value == hasher("http://example2.com");
	ok = ok && result[2].m_value == hasher("http://example.com");

	FullTextIndex fti5("test_db_3");

	result = fti5.search("hej");
	ok = ok && result.size() == 3;
	ok = ok && result[0].m_value == hasher("http://example3.com");
	ok = ok && result[1].m_value == hasher("http://example2.com");
	ok = ok && result[2].m_value == hasher("http://example.com");
	fti5.truncate();

	return ok;
}
