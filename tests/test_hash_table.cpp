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

#include <boost/test/unit_test.hpp>
#include "hash_table/hash_table.h"
#include "hash_table/builder.h"
#include "hash_table_helper/hash_table_helper.h"

#include "hash_table2/hash_table.h"
#include "hash_table2/builder.h"
#include "indexer/merger.h"

#include <set>

BOOST_AUTO_TEST_SUITE(test_hash_table)

BOOST_AUTO_TEST_CASE(single_shard_add) {

	hash_table_helper::truncate("test_index");

	{
		hash_table2::hash_table_shard_builder idx("test_index", 0);

		idx.truncate();

		idx.add(123, "hello world");
		idx.append();
		idx.merge();
	}

	{
		hash_table2::hash_table_shard idx("test_index", 0);

		BOOST_CHECK_EQUAL(idx.find(123), "hello world");
	}

}

BOOST_AUTO_TEST_CASE(single_shard_add_versioned) {

	{
		hash_table2::hash_table_shard_builder idx("test_index", 0);

		idx.truncate();

		idx.add(123, "hello world", 5);
		idx.append();
		idx.merge();
		idx.add(123, "new value", 6);
		idx.append();
		idx.merge();
		idx.add(123, "old value", 4);
		idx.append();
		idx.merge();

		idx.add(123, "old value 2", 3);
		idx.add(123, "newest value", 7);
		idx.append();
		idx.merge();
	}

	{
		hash_table2::hash_table_shard idx("test_index", 0);

		BOOST_CHECK_EQUAL(idx.find(123), "newest value");
	}

}

BOOST_AUTO_TEST_CASE(single_shard_add_versioned2) {

	{
		hash_table2::hash_table_shard_builder idx("test_index", 0);

		idx.truncate();

		idx.add(101, "an old value", 1000);
		idx.append();
		idx.merge();
		idx.optimize();
		idx.add(101, "another old value", 1000);
		idx.append();
		idx.merge();
		idx.optimize();
		idx.add(101, "a new value", 1001);
		idx.append();
		idx.merge();
		idx.optimize();
		idx.add(101, "an older value", 999);
		idx.append();
		idx.merge();
		idx.optimize();
	}

	{
		hash_table2::hash_table_shard idx("test_index", 0);

		BOOST_CHECK_EQUAL(idx.find(101), "a new value");
	}

}

BOOST_AUTO_TEST_CASE(add_to_hash_table) {

	hash_table_helper::truncate("test_index");

	{
		hash_table2::builder idx("test_index", 43);

		idx.truncate();

		// Add 1000 elements.
		for (size_t i = 0; i < 1000; i++) {
			idx.add(i, "Random test data with id: " + std::to_string(i));
		}

		idx.merge();
	}

	{
		hash_table2::hash_table hash_table("test_index", 43);

		for (size_t i = 0; i < 1000; i++) {
			BOOST_CHECK_EQUAL(hash_table.find(i), "Random test data with id: " + std::to_string(i));
		}
	}

	{
		hash_table2::builder idx("test_index", 43);

		idx.truncate();

		// Add 1000 elements.
		for (size_t i = 1000; i < 2000; i++) {
			idx.add(i, "Random test data with id: " + std::to_string(i));
		}

		idx.merge();
	}

	{
		hash_table2::hash_table hash_table("test_index", 43);

		for (size_t i = 1000; i < 2000; i++) {
			BOOST_CHECK_EQUAL(hash_table.find(i), "Random test data with id: " + std::to_string(i));
		}
	}

}

BOOST_AUTO_TEST_CASE(add_to_hash_table_reverse) {

	hash_table_helper::truncate("test_index");

	{
		hash_table2::builder idx("test_index", 17);

		idx.truncate();

		// Add 1000 elements.
		for (size_t i = 100000; i < 200000; i++) {
			idx.add(i, "Random test data with id: " + std::to_string(i));
		}

		idx.merge();
	}

	{
		hash_table2::hash_table hash_table("test_index", 17);

		BOOST_CHECK_EQUAL(hash_table.size(), 100000);
	}

	{
		// Add more elements.
		hash_table2::builder idx("test_index", 17);

		// Add 1000 elements.
		for (size_t i = 0; i < 100000; i++) {
			idx.add(i, "Random test data with id: " + std::to_string(i));
		}

		idx.merge();
	}

	{
		hash_table2::hash_table hash_table("test_index", 17);

		BOOST_CHECK_EQUAL(hash_table.size(), 200000);
	}

}

BOOST_AUTO_TEST_CASE(optimize) {

	hash_table_helper::truncate("test_index");

	size_t shard_size = 0;
	size_t shard_file_size = 0;

	{
		hash_table2::hash_table_shard_builder builder("test_index", 0);

		builder.add(1, "data element 1 v1");
		builder.add(2, "data element 2 v1");
		builder.add(3, "data element 3 v1");

		builder.append();
		builder.merge();

		hash_table2::hash_table_shard shard("test_index", 0);
		shard_size = shard.size();
		shard_file_size = shard.file_size();
	}

	{
		// Add some more elements with identical keys.
		hash_table2::hash_table_shard_builder builder("test_index", 0);

		builder.add(1, "data element 1 v2");
		builder.add(2, "data element 2 v2");
		builder.add(3, "data element 3 v2");

		builder.append();
		builder.merge();

		builder.optimize();

		hash_table2::hash_table_shard shard("test_index", 0);

		BOOST_CHECK_EQUAL(shard.size(), shard_size);
		BOOST_CHECK_EQUAL(shard.file_size(), shard_file_size);

		BOOST_CHECK_EQUAL(shard.find(1), "data element 1 v2");
		BOOST_CHECK_EQUAL(shard.find(2), "data element 2 v2");
		BOOST_CHECK_EQUAL(shard.find(3), "data element 3 v2");
	}
}

BOOST_AUTO_TEST_CASE(optimize_empty) {

	hash_table_helper::truncate("main_index");

	hash_table2::hash_table_shard_builder idx("main_index", 0);
	idx.optimize();

}

BOOST_AUTO_TEST_CASE(conditional) {

	hash_table_helper::truncate("main_index");

	{

		hash_table2::builder ht("main_index", 10);

		ht.truncate();

		ht.add(101, "an old value", 1000);
		ht.add(101, "another old value", 1000);
		ht.add(101, "a new value", 1001);
		ht.add(101, "an older value", 999);

		ht.merge();
	}

	{
		hash_table2::hash_table ht("main_index", 10);

		std::string value = ht.find(101);

		BOOST_CHECK_EQUAL(value, "a new value");
	}

}

BOOST_AUTO_TEST_CASE(conditional2) {

	hash_table_helper::truncate("main_index");

	{

		hash_table2::builder ht("main_index", 10);

		ht.truncate();

		// Merge between each. Should still get the same value.

		ht.add(101, "an old value", 1000);
		ht.merge();
		ht.add(101, "another old value", 1000);
		ht.merge();
		ht.add(101, "a new value", 1001);
		ht.merge();
		ht.add(101, "an older value", 999);
		ht.merge();
	}

	{
		hash_table2::hash_table ht("main_index", 10);

		std::string value = ht.find(101);

		BOOST_CHECK_EQUAL(value, "a new value");
	}

}

BOOST_AUTO_TEST_CASE(more_tests) {

	hash_table_helper::truncate("main_index");

	{
		hash_table2::builder ht("main_index", 10);

		ht.truncate();

		ht.add(101, "first value", 1000);
		ht.add(101, "second value", 1001);
		ht.add(101, "third value", 1002);

		ht.add(102, "first value", 1000);
		ht.add(102, "second value", 1001);
		ht.add(102, "third value", 1002);

		ht.add(103, "first value", 1);
		ht.add(103, "second value", 100000);
		ht.add(103, "third value", 99999999999);

		ht.add(50, "third value");

		ht.merge();
	}

	{
		hash_table2::hash_table ht("main_index", 10);

		BOOST_CHECK_EQUAL(ht.find(101), "third value");
		BOOST_CHECK_EQUAL(ht.find(102), "third value");
		BOOST_CHECK_EQUAL(ht.find(103), "third value");
		BOOST_CHECK_EQUAL(ht.find(50), "third value");
	}

}

BOOST_AUTO_TEST_CASE(for_each) {

	hash_table_helper::truncate("main_index");

	{
		hash_table2::builder ht("main_index", 10);

		ht.truncate();

		ht.add(101, "first value", 1000);
		ht.merge();
		ht.add(101, "second value", 1001);
		ht.merge();
		ht.add(101, "third value", 1002);

		ht.add(102, "first value", 1000);
		ht.merge();
		ht.add(102, "second value", 1001);
		ht.merge();
		ht.add(102, "third value", 1002);

		ht.add(103, "first value", 1);
		ht.merge();
		ht.add(103, "second value", 100000);
		ht.merge();
		ht.add(103, "third value", 99999999999);

		ht.add(50, "third value");

		ht.merge();
	}

	{
		hash_table2::hash_table ht("main_index", 10);

		std::set<uint64_t> keys;
		std::set<std::string> values;
		ht.for_each([&keys, &values](uint64_t key, const std::string &val) {
			keys.insert(key);
			values.insert(val);
		});

		BOOST_CHECK_EQUAL(keys.size(), 4);
		BOOST_CHECK_EQUAL(values.size(), 1);

		for (const auto &val : values) {
			BOOST_CHECK_EQUAL(val, "third value");
		}
	}

}

BOOST_AUTO_TEST_CASE(larger_test) {

	{
		indexer::merger::start_merge_thread();

		hash_table2::builder ht("main_index", 10);

		ht.truncate();

		for (size_t key = 1000; key < 10000; key++) {
			ht.add(key, std::string(key, 'x'));
		}

		for (size_t key = 1000; key < 10000; key++) {
			ht.add(key, std::string(key, 'y'), 1);
		}

		indexer::merger::stop_merge_thread();
	}

	{
		indexer::merger::start_merge_thread();

		hash_table2::builder ht("main_index", 10);

		for (size_t key = 1000; key < 10000; key++) {
			ht.add(key, std::string(key, 'z'), 2);
		}

		indexer::merger::stop_merge_thread();
	}

	{
		indexer::merger::start_merge_thread();

		hash_table2::builder ht("main_index", 10);

		for (size_t key = 1000; key < 10000; key++) {
			ht.add(key, std::string(key, 'a'), 2);
		}

		indexer::merger::stop_merge_thread();
	}

	{
		hash_table2::builder ht("main_index", 10);
		ht.optimize();
	}

	{
		hash_table2::hash_table ht("main_index", 10);

		for (size_t key = 1000; key < 10000; key++) {
			BOOST_REQUIRE_EQUAL(ht.find(key), std::string(key, 'a'));
		}

		std::map<uint64_t, std::vector<std::string>> vals;
		ht.for_each([&vals](uint64_t key, const std::string &val) {
			vals[key].push_back(val);
		});

		for (const auto &iter : vals) {
			BOOST_REQUIRE_EQUAL(iter.second.size(), 1);
			BOOST_REQUIRE_EQUAL(iter.second[0], std::string(iter.first, 'a'));
		}
	}

}

BOOST_AUTO_TEST_CASE(merge_with) {

	{
		hash_table2::builder ht("main_index", 11);

		ht.truncate();

		ht.add(123, "a1", 10);
		ht.add(1230, "a2", 10);
		ht.add(1231, "a3", 10);
		ht.add(1231, "a3_n2", 11);

		ht.add(3828540, "a4", 10);
		ht.add(2234645, "a5", 10);
		ht.add(8424878, "a6", 10);
		ht.add(4174861, "a7", 10);
		ht.add(7013344, "a8", 10);

		ht.merge();
	}

	{
		hash_table2::builder ht("main_index2", 11);

		ht.truncate();

		ht.add(123, "b1", 11);
		ht.add(1230, "b2", 12);
		ht.add(1231, "b3", 9);
		ht.add(1231, "b3", 8);

		ht.add(8321508, "b4", 10);
		ht.add(7309646, "b5", 10);
		ht.add(2809224, "b6", 10);
		ht.add(6543485, "b7", 10);
		ht.add(6078858, "b8", 10);

		ht.merge();
	}

	{
		hash_table2::builder ht1("main_index", 11);
		hash_table2::builder ht2("main_index2", 11);

		ht1.merge_with(ht2);
	}

	{
		hash_table2::hash_table ht("main_index", 11);

		BOOST_CHECK_EQUAL(ht.find(123), "b1");
		BOOST_CHECK_EQUAL(ht.find(1230), "b2");
		BOOST_CHECK_EQUAL(ht.find(1231), "a3_n2");
		BOOST_CHECK_EQUAL(ht.find(6543485), "b7");
		BOOST_CHECK_EQUAL(ht.find(2234645), "a5");
	}

}

BOOST_AUTO_TEST_CASE(merge_with_files) {

	{
		hash_table2::builder ht("main_index", 1);

		ht.truncate();

		ht.add(123, "a1", 10);
		ht.add(1230, "a2", 10);
		ht.add(1231, "a3", 10);
		ht.add(1231, "a3_n2", 11);

		ht.add(3828540, "a4", 10);
		ht.add(2234645, "a5", 10);
		ht.add(8424878, "a6", 10);
		ht.add(4174861, "a7", 10);
		ht.add(7013344, "a8", 10);

		ht.merge();
	}

	{
		hash_table2::builder ht("main_index2", 1);

		ht.truncate();

		ht.add(123, "b1", 11);
		ht.add(1230, "b2", 12);
		ht.add(1231, "b3", 9);
		ht.add(1231, "b3", 8);

		ht.add(8321508, "b4", 10);
		ht.add(7309646, "b5", 10);
		ht.add(2809224, "b6", 10);
		ht.add(6543485, "b7", 10);
		ht.add(6078858, "b8", 10);

		ht.merge();
	}

	{
		hash_table2::builder ht("main_index2", 1);

		ht.get_shard(0)->merge_with("/mnt/0/hash_table/ht_main_index_0.pos", "/mnt/0/hash_table/ht_main_index_0.data");
	}
	{
		hash_table2::hash_table ht("main_index2", 1);

		BOOST_CHECK_EQUAL(ht.find(123), "b1");
		BOOST_CHECK_EQUAL(ht.find(1230), "b2");
		BOOST_CHECK_EQUAL(ht.find(1231), "a3_n2");
		BOOST_CHECK_EQUAL(ht.find(6543485), "b7");
		BOOST_CHECK_EQUAL(ht.find(2234645), "a5");
	}

}

BOOST_AUTO_TEST_SUITE_END()
