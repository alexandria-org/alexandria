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

#include "text/Text.h"
#include "hash/Hash.h"
#include "full_text/FullText.h"

BOOST_AUTO_TEST_SUITE(n_gram)

BOOST_AUTO_TEST_CASE(words_to_ngram) {
	vector<uint64_t> ngrams;
	Text::words_to_ngram_hash({"the", "quick", "brown", "fox", "jumps", "over", "the", "lazy", "dog"}, 3, [&ngrams](const uint64_t hash) {
		ngrams.push_back(hash);
	});

	BOOST_CHECK_EQUAL(ngrams[0], Hash::str("the"));
	BOOST_CHECK_EQUAL(ngrams[1], Hash::str("the quick"));
	BOOST_CHECK_EQUAL(ngrams[2], Hash::str("the quick brown"));

	BOOST_CHECK_EQUAL(ngrams[3], Hash::str("quick"));
	BOOST_CHECK_EQUAL(ngrams[4], Hash::str("quick brown"));
	BOOST_CHECK_EQUAL(ngrams[5], Hash::str("quick brown fox"));

	BOOST_CHECK_EQUAL(ngrams[6], Hash::str("brown"));
	BOOST_CHECK_EQUAL(ngrams[7], Hash::str("brown fox"));
	BOOST_CHECK_EQUAL(ngrams[8], Hash::str("brown fox jumps"));

	BOOST_CHECK_EQUAL(ngrams[18], Hash::str("the"));
	BOOST_CHECK_EQUAL(ngrams[19], Hash::str("the lazy"));
	BOOST_CHECK_EQUAL(ngrams[20], Hash::str("the lazy dog"));

	BOOST_CHECK_EQUAL(ngrams[21], Hash::str("lazy"));
	BOOST_CHECK_EQUAL(ngrams[22], Hash::str("lazy dog"));
	BOOST_CHECK_EQUAL(ngrams[23], Hash::str("dog"));

	BOOST_CHECK_EQUAL(ngrams.size(), 24);

}

BOOST_AUTO_TEST_CASE(n_gram) {

	size_t initial_results_per_section = Config::ft_max_results_per_section;

	Config::n_grams = 5;
	Config::ft_max_results_per_section = 1000;

	SearchAllocation::Allocation *allocation = SearchAllocation::create_allocation();

	FullText::truncate_url_to_domain("main_index");
	FullText::truncate_index("test_main_index");

	HashTableHelper::truncate("test_main_index");

	{
		// Index full text
		FullText::index_single_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-07");
	}

	{
		// Count elements in hash tables.
		HashTable hash_table("test_main_index");

		// Make searches.
		vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("test_main_index");

		const string query = "The latter is commonly used";
		struct SearchMetric metric;

		vector<FullTextRecord> results = SearchEngine::search_exact(allocation->storage, index_array, query, 1000, metric);

		BOOST_CHECK_EQUAL(results.size(), 1);
		BOOST_CHECK_EQUAL(metric.m_total_found, 1);
		BOOST_CHECK_EQUAL(metric.m_link_domain_matches, 0);
		BOOST_CHECK_EQUAL(metric.m_link_url_matches, 0);
		BOOST_CHECK_EQUAL(results[0].m_value, URL("https://en.wikipedia.org/wiki/A").hash());

		FullText::delete_index_array<FullTextRecord>(index_array);
	}

	SearchAllocation::delete_allocation(allocation);

	Config::n_grams = 1;
	Config::ft_max_results_per_section = initial_results_per_section;
}

BOOST_AUTO_TEST_SUITE_END()
