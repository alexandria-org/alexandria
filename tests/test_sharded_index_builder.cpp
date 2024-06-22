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
#include "indexer/index_manager.h"
#include "indexer/sharded_index_builder.h"
#include "indexer/sharded_index.h"
#include "indexer/merger.h"
#include "text/text.h"
#include "algorithm/hash.h"
#include "transfer/transfer.h"

BOOST_AUTO_TEST_SUITE(test_sharded_index_builder)

BOOST_AUTO_TEST_CASE(test_sharded_index_builder) {

	{
		indexer::sharded_index_builder<indexer::generic_record> idx("test_index", 10);

		idx.truncate();

		idx.add(101, indexer::generic_record(1000, 1.0f));
		idx.add(102, indexer::generic_record(1001, 1.0f));

		idx.append();
		idx.merge();
	}

	{
		indexer::sharded_index<indexer::generic_record> idx("test_index", 10);
		vector<indexer::generic_record> res = idx.find(101);

		BOOST_REQUIRE(res.size() == 1);
		BOOST_CHECK(res[0].m_value == 1000);
	}

}

BOOST_AUTO_TEST_CASE(test_group_by) {

	using indexer::domain_link_record;

	{
		indexer::sharded_index_builder<domain_link_record> idx("test_index", 1);

		idx.truncate();

		idx.add(101, domain_link_record(1000, 1.0f, 200));
		idx.add(101, domain_link_record(1004, 1.0f, 300));
		idx.add(101, domain_link_record(1001, 1.0f, 200));
		idx.add(101, domain_link_record(1003, 1.0f, 300));
		idx.add(101, domain_link_record(1002, 1.0f, 200));

		idx.add(102, domain_link_record(1000, 1.0f, 200));
		idx.add(102, domain_link_record(1001, 1.0f, 200));
		idx.add(102, domain_link_record(1005, 1.0f, 300));
		idx.add(102, domain_link_record(1002, 1.0f, 200));

		idx.add(103, domain_link_record(1000, 1.0f, 200));
		idx.add(103, domain_link_record(1001, 1.0f, 200));
		idx.add(103, domain_link_record(1004, 1.0f, 300));
		idx.add(103, domain_link_record(1002, 1.0f, 200));

		idx.append();
		idx.merge();
		idx.optimize();
	}

	{
		indexer::sharded_index<domain_link_record> idx("test_index", 1);

		auto identity = [](float score) {
			return score;
		};
		std::vector<size_t> counts;
		vector<domain_link_record> res = idx.find_group_by({101, 102}, identity, counts);

		BOOST_REQUIRE(res.size() == 1);
		BOOST_CHECK(res[0].m_score == 3.0f);
		BOOST_CHECK(counts[0] == 3);
	}

	{
		indexer::sharded_index<domain_link_record> idx("test_index", 1);
		auto times_two = [](float score) {
			return 2.0f * score;
		};
		std::vector<size_t> counts;
		vector<domain_link_record> res = idx.find_group_by({101, 103}, times_two, counts);

		BOOST_REQUIRE(res.size() == 2);

		sort(res.begin(), res.end(), domain_link_record::storage_order());
		BOOST_CHECK(res[0].m_score == 2.0f * (3.0f));
		BOOST_CHECK(res[1].m_score == 2.0f * (1.0f));
		BOOST_CHECK(counts[0] == 3);
		BOOST_CHECK(counts[1] == 1);
	}

}

BOOST_AUTO_TEST_CASE(test_score_mod) {

	using indexer::domain_record;

	{
		indexer::sharded_index_builder<domain_record> idx("test_index", 1);

		idx.truncate();

		idx.add(101, domain_record(1000, 1.0f));
		idx.add(101, domain_record(1004, 1.0f));
		idx.add(101, domain_record(1001, 1.0f));
		idx.add(101, domain_record(1003, 1.0f));
		idx.add(101, domain_record(1002, 1.0f));

		idx.add(102, domain_record(1000, 1.0f));
		idx.add(102, domain_record(1001, 1.0f));
		idx.add(102, domain_record(1005, 1.0f));
		idx.add(102, domain_record(1002, 1.0f));

		idx.append();
		idx.merge();
		idx.optimize();
	}

	{
		/*
		 * intersected records will be in this order:
		 * 1000
		 * 1001
		 * 1002
		 *
		 * so score modification will take place in that order.
		 *
		 * */
		indexer::sharded_index<domain_record> idx("test_index", 1);
		uint64_t sum_id = 0;
		vector<domain_record> res = idx.find_top({101, 102}, 2,
				[&sum_id](const domain_record &val) -> float {
					return (float)(sum_id++);
				});

		BOOST_REQUIRE(res.size() == 2);
		BOOST_CHECK(res[0].m_score == 2.0f);
		BOOST_CHECK(res[0].m_value == 1002);
		BOOST_CHECK(res[1].m_score == 1.0f);
		BOOST_CHECK(res[1].m_value == 1001);
	}

}

BOOST_AUTO_TEST_SUITE_END()
