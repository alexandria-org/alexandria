
#include "test6.h"
#include "hash_table/HashTable.h"

using namespace std;

/*
 * Test Hash Table
 */
int test6_1(void) {
	int ok = 1;

	HashTable hash_table;
	hash_table.add(123, "hejsan");

	ok = ok && hash_table.find(123) == "hejsan";
	ok = ok && hash_table.find(1234) == "";

	hash_table.add(123, "testing");
	ok = ok && hash_table.find(123) == "testing";

	for (size_t i = 200; i < 1000; i++) {
		hash_table.add(i, "testing" + to_string(i));
	}

	for (size_t i = 200; i < 1000; i++) {
		ok = ok && (hash_table.find(i) == "testing" + to_string(i));
	}


	return ok;
}

