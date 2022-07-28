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
#include "config.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(test_config)

BOOST_AUTO_TEST_CASE(read_config) {
	config::read_config("../tests/test_config.conf");
	BOOST_CHECK_EQUAL(config::nodes_in_cluster, 3);
	BOOST_CHECK_EQUAL(config::node_id, 0);

	vector<string> batches{"ALEXANDRIA-MANUAL-01", "CC-MAIN-2021-25", "CC-MAIN-2021-31"};
	BOOST_CHECK(config::batches == batches);

	vector<string> link_batches{
		"CC-MAIN-2021-31",
        "CC-MAIN-2021-25",
        "CC-MAIN-2021-21",
        "CC-MAIN-2021-17",
        "CC-MAIN-2021-10",
        "CC-MAIN-2021-04",
        "CC-MAIN-2020-50",
        "CC-MAIN-2020-45"
	};
	BOOST_CHECK(config::link_batches == link_batches);
	BOOST_CHECK_EQUAL(config::worker_count, 8);
	BOOST_CHECK_EQUAL(config::query_max_words, 10);
	BOOST_CHECK_EQUAL(config::query_max_len, 200);
	BOOST_CHECK_EQUAL(config::deduplicate_domain_count, 5);
	BOOST_CHECK_EQUAL(config::pre_result_limit, 200000);
	BOOST_CHECK_EQUAL(config::result_limit, 1000);
	BOOST_CHECK_EQUAL(config::ft_max_sections, 4);
	BOOST_CHECK_EQUAL(config::ft_max_results_per_section, 2000000);

	config::read_config("../tests/test_config2.conf");
	BOOST_CHECK_EQUAL(config::nodes_in_cluster, 8);
	BOOST_CHECK_EQUAL(config::node_id, 1);

	vector<string> batches2{"ALEXANDRIA-MANUAL-02", "CC-MAIN-2021-20", "CC-MAIN-2021-30"};
	BOOST_CHECK(config::batches == batches2);

	vector<string> link_batches2{
		"CC-MAIN-2021-30",
        "CC-MAIN-2021-20",
        "CC-MAIN-2021-20",
        "CC-MAIN-2021-10",
        "CC-MAIN-2021-11",
        "CC-MAIN-2021-00",
        "CC-MAIN-2020-51",
        "CC-MAIN-2020-40"
	};
	BOOST_CHECK(config::link_batches == link_batches2);
	BOOST_CHECK_EQUAL(config::worker_count, 9);
	BOOST_CHECK_EQUAL(config::query_max_words, 100);
	BOOST_CHECK_EQUAL(config::query_max_len, 0);
	BOOST_CHECK_EQUAL(config::deduplicate_domain_count, 5000);
	BOOST_CHECK_EQUAL(config::pre_result_limit, 2);
	BOOST_CHECK_EQUAL(config::result_limit, 10);
	BOOST_CHECK_EQUAL(config::ft_max_sections, 2);
	BOOST_CHECK_EQUAL(config::ft_max_results_per_section, 20);

	BOOST_CHECK_EQUAL(config::n_grams, 5);
	BOOST_CHECK_EQUAL(config::index_snippets, false);

	config::read_config("../tests/test_config.conf");
}

BOOST_AUTO_TEST_SUITE_END()
