
#include "test6.h"
#include "hash_table/HashTable.h"

using namespace std;

/*
 * Test Hash Table
 */
int test6_1(void) {
	int ok = 1;

	HashTable hash_table;
	hash_table.wait_for_start();
	hash_table.add(123, "hejsan");

	ok = ok && hash_table.find(123) == "hejsan";
	ok = ok && hash_table.find(1234) == "";

	return ok;
}

