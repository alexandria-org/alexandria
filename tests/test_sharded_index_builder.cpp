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
#include "indexer/sharded_index_builder.h"
#include "indexer/sharded_index.h"
#include "indexer/level.h"
#include "text/text.h"
#include "algorithm/hash.h"

BOOST_AUTO_TEST_SUITE(test_sharded_index_builder)

BOOST_AUTO_TEST_CASE(test_sharded_index_builder) {

	{
		indexer::sharded_index_builder<indexer::generic_record> idx("test_index", 10);

		idx.truncate();

		idx.add(101, indexer::generic_record(1000, 1.0f));
		idx.add(102, indexer::generic_record(1001, 1.0f));

		idx.append();
		idx.merge();
		idx.calculate_scores(indexer::algorithm::bm25);

		BOOST_CHECK(idx.num_documents() == 2);
		BOOST_CHECK(idx.document_size(1000) == 1);
	}

	{
		indexer::sharded_index<indexer::generic_record> idx("test_index", 10);
		vector<indexer::generic_record> res = idx.find(101);

		BOOST_REQUIRE(res.size() == 1);
		BOOST_CHECK(res[0].m_value == 1000);
	}

}

BOOST_AUTO_TEST_CASE(test_sharded_index_builder_bm25) {

	const string domain1 = "heroes 3 wiki heroes 3 wiki heroes 3 wiki heroes 3 wiki heroes 3 wiki";
	const string domain2 = "the most exclusive news about the tv series heroes and its 3 episodes "
		"can be found in the wiki";
	const string domain3 = "one of the best things with being a programmer is that you can also play heroes 3 "
		"and write wiki pages";

	{
		indexer::sharded_index_builder<indexer::generic_record> idx("test_index", 10);

		idx.truncate();

		// index domain1
		for (const auto &word : text::get_full_text_words(domain1)) {
			const size_t key = algorithm::hash(word);
			indexer::generic_record record(1, 1.0f);
			idx.add(key, record);
		}

		// index domain2
		for (const auto &word : text::get_full_text_words(domain2)) {
			const size_t key = algorithm::hash(word);
			indexer::generic_record record(2, 1.0f);
			idx.add(key, record);
		}

		// index domain3
		for (const auto &word : text::get_full_text_words(domain3)) {
			const size_t key = algorithm::hash(word);
			indexer::generic_record record(3, 1.0f);
			idx.add(key, record);
		}

		idx.append();
		idx.merge();

		// Memory footprint should be same before and after calculate_scores.
		const size_t mem_before = memory::allocated_memory();
		idx.calculate_scores(indexer::algorithm::bm25);
		const size_t mem_after = memory::allocated_memory();
		BOOST_CHECK_EQUAL(mem_before, mem_after);

		BOOST_CHECK(idx.num_documents() == 3);
		BOOST_CHECK(idx.document_size(1) == 15);
	}

	{
		indexer::sharded_index<indexer::generic_record> idx("test_index", 10);
		vector<indexer::generic_record> res = idx.find(algorithm::hash("heroes"));

		BOOST_REQUIRE(res.size() == 3);
		BOOST_CHECK(res[0].m_value == 1);
	}

}

BOOST_AUTO_TEST_SUITE_END()
