/*
 * MIT License
 *
 * Alexandria.org
 *
 * Copyright (c) 2021 Josef Cullhed, <info@alexandria.org>, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "hash_table/hash_table.h"
#include "hash_table_helper/hash_table_helper.h"

BOOST_AUTO_TEST_SUITE(hash_table)

BOOST_AUTO_TEST_CASE(add_to_hash_table) {

	hash_table_helper::truncate("test_index");

	{
		vector<hash_table_shard_builder *> shards = hash_table_helper::create_shard_builders("test_index");

		// Add 1000 elements.
		for (size_t i = 0; i < 1000; i++) {
			hash_table_helper::add_data(shards, i, "Random test data with id: " + std::to_string(i));
		}

		hash_table_helper::write(shards);
		hash_table_helper::sort(shards);

		hash_table_helper::delete_shard_builders(shards);
	}

	{
		hash_table hash_table("test_index");

		BOOST_CHECK_EQUAL(hash_table.size(), 1000);
	}

	{
		// Add more elements.
		vector<hash_table_shard_builder *> shards = hash_table_helper::create_shard_builders("test_index");

		// Add 1000 elements.
		for (size_t i = 1000; i < 2000; i++) {
			hash_table_helper::add_data(shards, i, "Random test data with id: " + std::to_string(i));
		}

		hash_table_helper::write(shards);
		hash_table_helper::sort(shards);

		hash_table_helper::delete_shard_builders(shards);
	}

	{
		hash_table hash_table("test_index");

		BOOST_CHECK_EQUAL(hash_table.size(), 2000);
	}

}

BOOST_AUTO_TEST_CASE(add_to_hash_table_reverse) {

	hash_table_helper::truncate("test_index");

	{
		vector<hash_table_shard_builder *> shards = hash_table_helper::create_shard_builders("test_index");

		// Add 1000 elements.
		for (size_t i = 100000; i < 200000; i++) {
			hash_table_helper::add_data(shards, i, "Random test data with id: " + std::to_string(i));
		}

		hash_table_helper::write(shards);
		hash_table_helper::sort(shards);

		hash_table_helper::delete_shard_builders(shards);
	}

	{
		hash_table hash_table("test_index");

		BOOST_CHECK_EQUAL(hash_table.size(), 100000);
	}

	{
		// Add more elements.
		vector<hash_table_shard_builder *> shards = hash_table_helper::create_shard_builders("test_index");

		// Add 1000 elements.
		for (size_t i = 0; i < 100000; i++) {
			hash_table_helper::add_data(shards, i, "Random test data with id: " + std::to_string(i));
		}

		hash_table_helper::write(shards);
		hash_table_helper::sort(shards);

		hash_table_helper::delete_shard_builders(shards);
	}

	{
		hash_table hash_table("test_index");

		BOOST_CHECK_EQUAL(hash_table.size(), 200000);
	}

}

BOOST_AUTO_TEST_CASE(optimize) {

	hash_table_helper::truncate("test_index");

	size_t shard_size = 0;
	size_t shard_file_size = 0;

	{
		hash_table_shard_builder builder("test_index", 0);

		builder.add(1, "data element 1 v1");
		builder.add(2, "data element 2 v1");
		builder.add(3, "data element 3 v1");

		builder.write();
		builder.sort();

		hash_table_shard shard("test_index", 0);
		shard_size = shard.size();
		shard_file_size = shard.file_size();
	}

	{
		// Add some more elements with identical keys.
		hash_table_shard_builder builder("test_index", 0);

		builder.add(1, "data element 1 v2");
		builder.add(2, "data element 2 v2");
		builder.add(3, "data element 3 v2");

		builder.write();
		builder.sort();

		builder.optimize();

		hash_table_shard shard("test_index", 0);

		BOOST_CHECK_EQUAL(shard.size(), shard_size);
		BOOST_CHECK_EQUAL(shard.file_size(), shard_file_size);

		BOOST_CHECK_EQUAL(shard.find(1), "data element 1 v2");
		BOOST_CHECK_EQUAL(shard.find(2), "data element 2 v2");
		BOOST_CHECK_EQUAL(shard.find(3), "data element 3 v2");
	}
}

BOOST_AUTO_TEST_CASE(optimize_empty) {

	hash_table_helper::truncate("main_index");

	vector<hash_table_shard_builder *> shards = hash_table_helper::create_shard_builders("main_index");
	hash_table_helper::optimize(shards);

}

BOOST_AUTO_TEST_SUITE_END()
