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
#include <mutex>
#include "indexer/sharded_builder.h"
#include "indexer/sharded.h"
#include "indexer/counted_index_builder.h"
#include "indexer/counted_index.h"
#include "indexer/counted_record.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(test_index_iteration)

BOOST_AUTO_TEST_CASE(test_index_iteration) {

	{
		indexer::sharded_builder<indexer::counted_index_builder, indexer::counted_record> idx("test_index", 10);
		idx.truncate();

		idx.add(100, indexer::counted_record(1000));
		idx.add(101, indexer::counted_record(1001));
		idx.add(101, indexer::counted_record(1002));
		idx.add(102, indexer::counted_record(1003));

		idx.append();
		idx.merge();
	}

	indexer::sharded<indexer::counted_index, indexer::counted_record> idx("test_index", 10);

	std::vector<uint64_t> found_keys;
	std::vector<uint64_t> found_values;
	std::mutex lock;
	idx.for_each([&lock, &found_keys, &found_values](uint64_t key, const std::vector<indexer::counted_record> &recs) {

		std::lock_guard grd(lock);

		found_keys.push_back(key);
		for (auto &rec : recs) {
			found_values.push_back(rec.m_value);
		}

	});

	std::sort(found_keys.begin(), found_keys.end());
	std::sort(found_values.begin(), found_values.end());

	BOOST_CHECK(found_keys[0] == 100);
	BOOST_CHECK(found_keys[1] == 101);
	BOOST_CHECK(found_keys[2] == 102);
	BOOST_CHECK(found_keys.size() == 3);

	BOOST_CHECK(found_values[0] == 1000);
	BOOST_CHECK(found_values[1] == 1001);
	BOOST_CHECK(found_values[2] == 1002);
	BOOST_CHECK(found_values[3] == 1003);
	BOOST_CHECK(found_values.size() == 4);

}

BOOST_AUTO_TEST_CASE(test_index_iteration2) {

	{
		indexer::sharded_builder<indexer::counted_index_builder, indexer::counted_record> idx("test_index", 10);
		idx.truncate();

		for (size_t i = 1; i <= 10000; i++) {
			idx.add(i % 10, indexer::counted_record(i));
			idx.add(i % 100, indexer::counted_record(i));
			idx.add(i % 7, indexer::counted_record(i));
			idx.add(i % 13, indexer::counted_record(i));
		}

		idx.append();
		idx.merge();
	}

	indexer::sharded<indexer::counted_index, indexer::counted_record> idx("test_index", 10);

	std::map<uint64_t, std::vector<size_t>> records;
	std::mutex lock;
	idx.for_each([&lock, &records](uint64_t key, const std::vector<indexer::counted_record> &recs) {

		std::lock_guard grd(lock);

		for (auto &rec : recs) {
			records[key].push_back(rec.m_value);
		}

	});

	for (size_t i = 1; i <= 10000; i++) {
		BOOST_CHECK(std::find(records[i % 10].begin(), records[i % 10].end(), i) != records[i % 10].end());
		BOOST_CHECK(std::find(records[i % 100].begin(), records[i % 100].end(), i) != records[i % 100].end());
		BOOST_CHECK(std::find(records[i % 7].begin(), records[i % 7].end(), i) != records[i % 7].end());
		BOOST_CHECK(std::find(records[i % 13].begin(), records[i % 13].end(), i) != records[i % 13].end());
	}

}

BOOST_AUTO_TEST_SUITE_END()
