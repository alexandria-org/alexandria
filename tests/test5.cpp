

#include "test5.h"
#include "FullTextIndex.h"

using namespace std;

/*
 * Test Search API
 */
int test5_1(void) {
	int ok = 1;

	FullTextIndex fti;
	std::hash<string> hasher;
	//fti.add("http://example.com", "Hej hopp josef");
	vector<FullTextResult> result = fti.search("hej");
	ok = ok && result.size() == 1;
	ok = ok && result[0].m_value == hasher("http://example.com");

	fti.save();

	return ok;
}
