
#include "test6.h"
#include "hash_table/HashTable.h"
#include "full_text/FullTextIndex.h"

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
	}

	{
		HashTable hash_table("test_index");
		hash_table.truncate();

		for (size_t i = 200; i < 210; i++) {
			hash_table.add(i, "testing" + to_string(i));
		}
	}

	{
		HashTable hash_table("test_index");
		for (size_t i = 200; i < 210; i++) {
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
	}

	return ok;
}

