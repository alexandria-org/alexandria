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
#include "file/file.h"
#include "indexer/index_builder.h"
#include "indexer/index.h"
#include "indexer/generic_record.h"
#include "indexer/value_record.h"

BOOST_AUTO_TEST_SUITE(test_index_builder)

BOOST_AUTO_TEST_CASE(test_merge_with) {

	file::delete_directory("./0/full_text/test_index");
	file::create_directory("./0/full_text/test_index");

	{
		indexer::index_builder<indexer::value_record> idx("test_index", 0, 1000);

		idx.add(123, indexer::value_record(1000));
		idx.add(123, indexer::value_record(1001));
		idx.add(124, indexer::value_record(1000));

		idx.append();
		idx.merge();
	}
	{
		indexer::index<indexer::value_record> idx("test_index", 0, 1000);

		auto res1 = idx.find(123);
		auto res2 = idx.find(124);

		BOOST_REQUIRE_EQUAL(res1.size(), 2);
		BOOST_REQUIRE_EQUAL(res2.size(), 1);

		BOOST_CHECK_EQUAL(res1[0].m_value, 1000);
		BOOST_CHECK_EQUAL(res1[1].m_value, 1001);
		BOOST_CHECK_EQUAL(res2[0].m_value, 1000);
	}
	{
		indexer::index_builder<indexer::value_record> idx("test_index", 8, 1000);

		idx.add(123, indexer::value_record(1002));
		idx.add(123, indexer::value_record(1003));
		idx.add(124, indexer::value_record(1010));
		idx.add(125, indexer::value_record(1011));

		idx.append();
		idx.merge();
	}

	{
		indexer::index_builder<indexer::value_record> idx1("test_index", 0, 1000);
		indexer::index<indexer::value_record> idx2("test_index", 8, 1000);

		idx1.merge_with(idx2);
	}

	{
		indexer::index<indexer::value_record> idx("test_index", 0, 1000);

		auto res1 = idx.find(123);
		auto res2 = idx.find(124);
		auto res3 = idx.find(125);

		BOOST_REQUIRE_EQUAL(res1.size(), 4);
		BOOST_REQUIRE_EQUAL(res2.size(), 2);
		BOOST_REQUIRE_EQUAL(res3.size(), 1);

		BOOST_CHECK_EQUAL(res1[0].m_value, 1000);
		BOOST_CHECK_EQUAL(res1[1].m_value, 1001);
		BOOST_CHECK_EQUAL(res1[2].m_value, 1002);
		BOOST_CHECK_EQUAL(res1[3].m_value, 1003);
		BOOST_CHECK_EQUAL(res2[0].m_value, 1000);
		BOOST_CHECK_EQUAL(res2[1].m_value, 1010);
		BOOST_CHECK_EQUAL(res3[0].m_value, 1011);
	}
}

BOOST_AUTO_TEST_CASE(test_merge_with2) {

	file::delete_directory("./0/full_text/test_index");
	file::create_directory("./0/full_text/test_index");

	{
		indexer::index_builder<indexer::value_record> idx("test_index", 0, 1000);

		idx.add(123, indexer::value_record(1000));
		idx.add(123, indexer::value_record(1001));
		idx.add(124, indexer::value_record(1000));

		idx.append();
		idx.merge();
	}
	{
		indexer::index<indexer::value_record> idx("test_index", 0, 1000);

		auto res1 = idx.find(123);
		auto res2 = idx.find(124);

		BOOST_REQUIRE_EQUAL(res1.size(), 2);
		BOOST_REQUIRE_EQUAL(res2.size(), 1);

		BOOST_CHECK_EQUAL(res1[0].m_value, 1000);
		BOOST_CHECK_EQUAL(res1[1].m_value, 1001);
		BOOST_CHECK_EQUAL(res2[0].m_value, 1000);
	}
	{
		indexer::index_builder<indexer::value_record> idx("test_index", 8, 1000);

		idx.add(123, indexer::value_record(1002));
		idx.add(123, indexer::value_record(1003));
		idx.add(124, indexer::value_record(1010));
		idx.add(125, indexer::value_record(1011));

		idx.append();
		idx.merge();
	}

	{
		indexer::index_builder<indexer::value_record> idx1("test_index", 0, 1000);
		indexer::index<indexer::value_record> idx2("test_index", 8, 1000);

		idx1.merge_with(idx2);
	}

	{
		indexer::index<indexer::value_record> idx("test_index", 0, 1000);

		auto res1 = idx.find(123);
		auto res2 = idx.find(124);
		auto res3 = idx.find(125);

		BOOST_REQUIRE_EQUAL(res1.size(), 4);
		BOOST_REQUIRE_EQUAL(res2.size(), 2);
		BOOST_REQUIRE_EQUAL(res3.size(), 1);

		BOOST_CHECK_EQUAL(res1[0].m_value, 1000);
		BOOST_CHECK_EQUAL(res1[1].m_value, 1001);
		BOOST_CHECK_EQUAL(res1[2].m_value, 1002);
		BOOST_CHECK_EQUAL(res1[3].m_value, 1003);
		BOOST_CHECK_EQUAL(res2[0].m_value, 1000);
		BOOST_CHECK_EQUAL(res2[1].m_value, 1010);
		BOOST_CHECK_EQUAL(res3[0].m_value, 1011);
	}
}

BOOST_AUTO_TEST_SUITE_END()
