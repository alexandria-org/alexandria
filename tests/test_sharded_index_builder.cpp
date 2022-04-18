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
#include "indexer/index_tree.h"
#include "indexer/sharded_index_builder.h"
#include "indexer/sharded_index.h"
#include "indexer/level.h"
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

		idx.add(101, domain_link_record(1000, 1.0f, 100, 200));
		idx.add(101, domain_link_record(1004, 1.0f, 120, 300));
		idx.add(101, domain_link_record(1001, 1.0f, 110, 200));
		idx.add(101, domain_link_record(1003, 1.0f, 120, 300));
		idx.add(101, domain_link_record(1002, 1.0f, 120, 200));

		idx.add(102, domain_link_record(1000, 1.0f, 100, 200));
		idx.add(102, domain_link_record(1001, 1.0f, 110, 200));
		idx.add(102, domain_link_record(1005, 1.0f, 120, 300));
		idx.add(102, domain_link_record(1002, 1.0f, 120, 200));

		idx.add(103, domain_link_record(1000, 1.0f, 100, 200));
		idx.add(103, domain_link_record(1001, 1.0f, 110, 200));
		idx.add(103, domain_link_record(1004, 1.0f, 120, 300));
		idx.add(103, domain_link_record(1002, 1.0f, 120, 200));

		idx.append();
		idx.merge();
		idx.optimize();
	}

	{
		indexer::sharded_index<domain_link_record> idx("test_index", 1);
		vector<domain_link_record> res = idx.find_group_by({101, 102},
				[](domain_link_record &a, const domain_link_record &b) {
					a.m_score += b.m_score;
				});

		BOOST_REQUIRE(res.size() == 1);
		BOOST_CHECK(res[0].m_score == 3.0f);
	}

	{
		indexer::sharded_index<domain_link_record> idx("test_index", 1);
		auto add_scores = [](domain_link_record &a, const domain_link_record &b) {
			a.m_score += b.m_score;
		};
		vector<domain_link_record> res = idx.find_group_by({101, 103}, add_scores);

		BOOST_REQUIRE(res.size() == 2);

		sort(res.begin(), res.end(), domain_link_record::storage_order());
		BOOST_CHECK(res[0].m_score == 3.0f);
		BOOST_CHECK(res[1].m_score == 1.0f);
	}

}

BOOST_AUTO_TEST_CASE(test_with_real_data) {

	const size_t mem_before = memory::allocated_memory();
	{
		vector<string> warc_paths = {"crawl-data/ALEXANDRIA-MANUAL-01/files/top_domains.txt.gz"};
		std::vector<std::string> local_files = transfer::download_gz_files_to_disk(warc_paths);

		indexer::index_tree idx_tree;

		indexer::domain_level domain_level;
		idx_tree.add_level(&domain_level);

		idx_tree.truncate();

		indexer::merger::start_merge_thread();
		idx_tree.add_index_files_threaded(local_files, 1);
		indexer::merger::stop_merge_thread();
	
		transfer::delete_downloaded_files(local_files);
	}
	const size_t mem_after = memory::allocated_memory();
	//BOOST_CHECK(mem_before == mem_after);
	cout << "diff: " << mem_after - mem_before << endl;

	{
		hash_table::hash_table ht("index_tree");
		indexer::index_tree idx_tree;
		indexer::domain_level domain_level;
		idx_tree.add_level(&domain_level);

		std::vector<indexer::return_record> res = idx_tree.find("microsoft corporation main site");

		BOOST_REQUIRE(res.size() == 1);
		const string host = ht.find(res[0].m_value);
		BOOST_CHECK(host == "microsoft.com");
	}
}

BOOST_AUTO_TEST_CASE(test_optimization) {
	{
		vector<string> warc_paths = {"crawl-data/ALEXANDRIA-MANUAL-01/files/50_top_domains.txt.gz"};
		std::vector<std::string> local_files = transfer::download_gz_files_to_disk(warc_paths);

		indexer::index_tree idx_tree;

		indexer::domain_level domain_level;
		idx_tree.add_level(&domain_level);

		idx_tree.truncate();

		indexer::merger::start_merge_thread();
		idx_tree.add_index_files_threaded(local_files, 1);
		indexer::merger::stop_merge_thread();
	
		transfer::delete_downloaded_files(local_files);

		idx_tree.merge();
	}

	{
		hash_table::hash_table ht("index_tree");
		indexer::index_tree idx_tree;
		indexer::domain_level domain_level;
		idx_tree.add_level(&domain_level);

		std::vector<indexer::return_record> res = idx_tree.find("the");

		BOOST_REQUIRE(res.size() > 0);

		// Check strict ordering of results.
		uint64_t prev_value = 0;
		for (auto &record : res) {
			BOOST_CHECK(record.m_value > prev_value);
			prev_value = record.m_value;
		}
	}

	{
		hash_table::hash_table ht("index_tree");
		indexer::index_tree idx_tree;
		indexer::domain_level domain_level;
		idx_tree.add_level(&domain_level);

		std::vector<indexer::return_record> res = idx_tree.find("the people");

		BOOST_REQUIRE(res.size() == 3);

		// Check strict ordering of results.
		uint64_t prev_value = 0;
		for (auto &record : res) {
			BOOST_CHECK(record.m_value > prev_value);
			prev_value = record.m_value;
		}
	}
}

BOOST_AUTO_TEST_SUITE_END()
