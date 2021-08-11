
#include "hash_table/HashTable.h"
#include "hash_table/HashTableHelper.h"

BOOST_AUTO_TEST_SUITE(hash_table)

BOOST_AUTO_TEST_CASE(add_to_hash_table) {

	HashTableHelper::truncate("test_index");

	{
		vector<HashTableShardBuilder *> shards = HashTableHelper::create_shard_builders("test_index");

		// Add 1000 elements.
		for (size_t i = 0; i < 1000; i++) {
			HashTableHelper::add_data(shards, i, "Random test data with id: " + to_string(i));
		}

		HashTableHelper::write(shards);
		HashTableHelper::sort(shards);

		HashTableHelper::delete_shard_builders(shards);
	}

	{
		HashTable hash_table("test_index");

		BOOST_CHECK_EQUAL(hash_table.size(), 1000);
	}

	{
		// Add more elements.
		vector<HashTableShardBuilder *> shards = HashTableHelper::create_shard_builders("test_index");

		// Add 1000 elements.
		for (size_t i = 1000; i < 2000; i++) {
			HashTableHelper::add_data(shards, i, "Random test data with id: " + to_string(i));
		}

		HashTableHelper::write(shards);
		HashTableHelper::sort(shards);

		HashTableHelper::delete_shard_builders(shards);
	}

	{
		HashTable hash_table("test_index");

		BOOST_CHECK_EQUAL(hash_table.size(), 2000);
	}

}

BOOST_AUTO_TEST_CASE(add_to_hash_table_reverse) {

	HashTableHelper::truncate("test_index");

	{
		vector<HashTableShardBuilder *> shards = HashTableHelper::create_shard_builders("test_index");

		// Add 1000 elements.
		for (size_t i = 100000; i < 200000; i++) {
			HashTableHelper::add_data(shards, i, "Random test data with id: " + to_string(i));
		}

		HashTableHelper::write(shards);
		HashTableHelper::sort(shards);

		HashTableHelper::delete_shard_builders(shards);
	}

	{
		HashTable hash_table("test_index");

		BOOST_CHECK_EQUAL(hash_table.size(), 100000);
	}

	{
		// Add more elements.
		vector<HashTableShardBuilder *> shards = HashTableHelper::create_shard_builders("test_index");

		// Add 1000 elements.
		for (size_t i = 0; i < 100000; i++) {
			HashTableHelper::add_data(shards, i, "Random test data with id: " + to_string(i));
		}

		HashTableHelper::write(shards);
		HashTableHelper::sort(shards);

		HashTableHelper::delete_shard_builders(shards);
	}

	{
		HashTable hash_table("test_index");

		BOOST_CHECK_EQUAL(hash_table.size(), 200000);
	}

}

BOOST_AUTO_TEST_SUITE_END();
