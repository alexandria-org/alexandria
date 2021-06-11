
#include "test6.h"
#include "hash_table/HashTable.h"
#include "full_text/FullTextIndex.h"

using namespace std;

/*
 * Test Hash Table
 */
int test6_1(void) {
	int ok = 1;

	HashTable hash_table("main_index");
	hash_table.add(123, "hejsan");

	ok = ok && hash_table.find(123) == "hejsan";
	ok = ok && hash_table.find(1234) == "";

	hash_table.add(123, "testing");
	ok = ok && hash_table.find(123) == "testing";

	for (size_t i = 200; i < 210; i++) {
		hash_table.add(i, "testing" + to_string(i));
	}

	for (size_t i = 200; i < 210; i++) {
		ok = ok && (hash_table.find(i) == "testing" + to_string(i));
	}

	return ok;
}

int test6_2(void) {
	int ok = 1;

	SubSystem *sub_system = new SubSystem();

	FullTextIndex fti("main_index");
	fti.upload(sub_system);
	fti.download(sub_system);

	HashTable hash_table("main_index");
	hash_table.upload(sub_system);
	hash_table.download(sub_system);

	delete sub_system;

	return ok;
}

