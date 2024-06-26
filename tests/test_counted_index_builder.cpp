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
#include "indexer/basic_index_builder.h"
#include "indexer/basic_index.h"
#include "indexer/counted_record.h"
#include "indexer/sharded_builder.h"
#include "indexer/sharded.h"

using namespace indexer;

BOOST_AUTO_TEST_SUITE(test_basic_index_builder)

BOOST_AUTO_TEST_CASE(test_case_1) {

	{
		basic_index_builder<counted_record> idx("test_index", 0);

		idx.truncate();

		idx.add(101, counted_record(1000, 1.0f));

		idx.append();
		idx.merge();
	}

	{
		basic_index<counted_record> idx("test_index", 0);

		std::vector<counted_record> res = idx.find(101);
		BOOST_REQUIRE(res.size() == 1);
		BOOST_CHECK(res[0].m_value == 1000);
		BOOST_CHECK(res[0].m_count == 1);
	}

}

BOOST_AUTO_TEST_CASE(test_case_2) {

	{
		basic_index_builder<counted_record> idx("test_index", 0);

		idx.truncate();

		idx.add(101, counted_record(1000));
		idx.add(101, counted_record(1000));

		idx.append();
		idx.merge();
	}

	{
		basic_index<counted_record> idx("test_index", 0);

		std::vector<counted_record> res = idx.find(101);
		BOOST_REQUIRE(res.size() == 1);
		BOOST_CHECK(res[0].m_value == 1000);
		BOOST_CHECK(res[0].m_count == 2);
	}

}

BOOST_AUTO_TEST_CASE(test_case_3) {

	{
		basic_index_builder<counted_record> idx("test_index", 0);

		idx.truncate();

		idx.add(101, counted_record(1000));
		idx.add(101, counted_record(1001));
		idx.add(101, counted_record(1000));

		idx.append();
		idx.merge();
	}

	{
		basic_index<counted_record> idx("test_index", 0);

		std::vector<counted_record> res = idx.find(101);
		BOOST_REQUIRE(res.size() == 2);
		BOOST_CHECK(res[0].m_value == 1000);
		BOOST_CHECK(res[0].m_count == 2);
		BOOST_CHECK(res[1].m_value == 1001);
		BOOST_CHECK(res[1].m_count == 1);
	}

}

BOOST_AUTO_TEST_CASE(test_case_4) {

	{
		sharded_builder<basic_index_builder, counted_record> idx("test_index", 10);

		idx.truncate();

		idx.add(101, indexer::counted_record(1000));
		idx.add(101, indexer::counted_record(1001));
		idx.add(101, indexer::counted_record(1000));
		idx.add(102, indexer::counted_record(1002));

		idx.append();
		idx.merge();

		BOOST_CHECK(idx.document_count() == 3);
	}

	{
		sharded<basic_index, counted_record> idx("test_index", 10);

		std::vector<counted_record> res = idx.find(101);
		BOOST_REQUIRE(res.size() == 2);
		BOOST_CHECK(res[0].m_value == 1000);
		BOOST_CHECK(res[0].m_count == 2);
		BOOST_CHECK(res[1].m_value == 1001);
		BOOST_CHECK(res[1].m_count == 1);
	}

}

BOOST_AUTO_TEST_SUITE_END()
