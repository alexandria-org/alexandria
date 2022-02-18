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

#include "indexer/index_builder.h"
#include "indexer/index.h"
#include "indexer/snippet.h"
#include "indexer/index_tree.h"
#include "parser/URL.h"

BOOST_AUTO_TEST_SUITE(index_array)

BOOST_AUTO_TEST_CASE(index) {

	struct record {

		uint64_t m_value;
		float m_score;

	};

	/*
	 * This is the simplest form. We can create an index and add records to it. Then search for the keys.
	 * */
	{
		indexer::index_builder<record> idx("test", 0);
		idx.truncate();

		idx.add(123, record{.m_value = 1, .m_score = 0.2f});
		idx.add(123, record{.m_value = 2, .m_score = 0.3f});

		idx.append();
		idx.merge();
	}

	{
		indexer::index<record> idx("test", 0);
		std::vector<record> res = idx.find(123);
		// Results are sorted by value.
		BOOST_CHECK_EQUAL(res[0].m_value, 1);
		BOOST_CHECK_EQUAL(res[1].m_value, 2);
	}

}

BOOST_AUTO_TEST_CASE(snippet) {

	struct record {

		uint64_t m_value;
		float m_score;

	};

	indexer::snippet<record> snippet("mukandengineers.com", "http://mukandengineers.com/", "Employing more than 200");
	auto tokens = snippet.tokens();

	BOOST_REQUIRE_EQUAL(tokens.size(), 4);
	BOOST_CHECK_EQUAL(tokens[0], Hash::str("employing"));
	BOOST_CHECK_EQUAL(tokens[1], Hash::str("more"));
	BOOST_CHECK_EQUAL(tokens[2], Hash::str("than"));
	BOOST_CHECK_EQUAL(tokens[3], Hash::str("200"));

	BOOST_CHECK_EQUAL(snippet.key(indexer::level::domain), Hash::str("mukandengineers.com"));
	BOOST_CHECK_EQUAL(snippet.key(indexer::level::url), URL("http://mukandengineers.com/").hash());
}

BOOST_AUTO_TEST_CASE(index_tree) {

	struct record {

		uint64_t m_value;
		float m_score;

	};

	{
		indexer::index_tree<record> idx_tree;

		idx_tree.add_level(indexer::level::domain);
		idx_tree.add_level(indexer::level::url);
		idx_tree.add_level(indexer::level::snippet);

		indexer::snippet<record> snippet("mukandengineers.com", "http://mukandengineers.com/", "Employing more than 200 engineers, the company has undertaken several challenging and prestigious projects across many industries in India and is today known for its skill and reliability in delivering quality services. The company is equipped with a whole range of construction machinery  including mobile cranes, gantry cranes, welding machines, concrete batching plants, transit mixers , electrical test and measuring instruments.");

		idx_tree.add_snippet(snippet);
		idx_tree.merge();

		std::vector<record> res = idx_tree.find("Employing more than");

		BOOST_REQUIRE_EQUAL(res.size(), 1);
		BOOST_CHECK_EQUAL(res[0].m_value, snippet.key(indexer::level::snippet));
	}

}

BOOST_AUTO_TEST_SUITE_END()
