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

#include "hash_table/HashTableHelper.h"
#include "api/Api.h"

BOOST_AUTO_TEST_SUITE(sections)

BOOST_AUTO_TEST_CASE(sections) {

	unsigned long long initial_nodes_in_cluster = Config::nodes_in_cluster;
	unsigned long long initial_results_per_section = Config::ft_max_results_per_section;
	unsigned long long initial_ft_max_sections = Config::ft_max_sections;
	unsigned long long initial_ft_section_depth = Config::ft_section_depth;
	Config::nodes_in_cluster = 1;
	Config::node_id = 0;
	Config::ft_max_results_per_section = 10;
	Config::ft_max_sections = 8;
	Config::ft_section_depth = 64;

	SearchAllocation::Allocation *allocation = SearchAllocation::create_allocation();

	FullText::truncate_url_to_domain("main_index");
	FullText::truncate_index("test_main_index");

	HashTableHelper::truncate("test_main_index");

	// Index full text
	{
		SubSystem *sub_system = new SubSystem();
		FullText::index_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-08", sub_system);
	}

	HashTable hash_table("test_main_index");
	vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("test_main_index");

	{
		stringstream response_stream;
		Api::search("site:en.wikipedia.org Wikipedia", hash_table, index_array, {}, {}, allocation, response_stream);

		string response = response_stream.str();

		Aws::Utils::Json::JsonValue json(response);

		auto v = json.View();

		BOOST_CHECK(v.ValueExists("status"));
		BOOST_CHECK_EQUAL(v.GetString("status"), "success");
		//BOOST_CHECK_EQUAL(v.GetInteger("total_found"), 1);
		BOOST_CHECK_EQUAL(v.GetInteger("total_url_links_found"), 0);

		BOOST_CHECK(v.ValueExists("results"));
		BOOST_CHECK_EQUAL(v.GetArray("results").GetLength(), 160);
	}

	// Reset.
	Config::nodes_in_cluster = initial_nodes_in_cluster;
	Config::node_id = 0;
	Config::ft_max_results_per_section = initial_results_per_section;
	Config::ft_max_sections = initial_ft_max_sections;
	Config::ft_section_depth = initial_ft_section_depth;
}

BOOST_AUTO_TEST_SUITE_END();
