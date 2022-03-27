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

#include "text/text.h"
#include "algorithm/hash.h"
#include "full_text/full_text.h"

BOOST_AUTO_TEST_SUITE(n_gram)

BOOST_AUTO_TEST_CASE(words_to_ngram) {
	vector<uint64_t> ngrams;
	text::words_to_ngram_hash({"the", "quick", "brown", "fox", "jumps", "over", "the", "lazy", "dog"}, 3, [&ngrams](const uint64_t hash) {
		ngrams.push_back(hash);
	});

	BOOST_CHECK_EQUAL(ngrams[0], algorithm::hash("the"));
	BOOST_CHECK_EQUAL(ngrams[1], algorithm::hash("the quick"));
	BOOST_CHECK_EQUAL(ngrams[2], algorithm::hash("the quick brown"));

	BOOST_CHECK_EQUAL(ngrams[3], algorithm::hash("quick"));
	BOOST_CHECK_EQUAL(ngrams[4], algorithm::hash("quick brown"));
	BOOST_CHECK_EQUAL(ngrams[5], algorithm::hash("quick brown fox"));

	BOOST_CHECK_EQUAL(ngrams[6], algorithm::hash("brown"));
	BOOST_CHECK_EQUAL(ngrams[7], algorithm::hash("brown fox"));
	BOOST_CHECK_EQUAL(ngrams[8], algorithm::hash("brown fox jumps"));

	BOOST_CHECK_EQUAL(ngrams[18], algorithm::hash("the"));
	BOOST_CHECK_EQUAL(ngrams[19], algorithm::hash("the lazy"));
	BOOST_CHECK_EQUAL(ngrams[20], algorithm::hash("the lazy dog"));

	BOOST_CHECK_EQUAL(ngrams[21], algorithm::hash("lazy"));
	BOOST_CHECK_EQUAL(ngrams[22], algorithm::hash("lazy dog"));
	BOOST_CHECK_EQUAL(ngrams[23], algorithm::hash("dog"));

	BOOST_CHECK_EQUAL(ngrams.size(), 24);

}

BOOST_AUTO_TEST_CASE(n_gram) {

	size_t initial_results_per_section = config::ft_max_results_per_section;

	config::n_grams = 5;
	config::ft_max_results_per_section = 1000;

	search_allocation::allocation *allocation = search_allocation::create_allocation();

	full_text::truncate_url_to_domain("main_index");
	full_text::truncate_index("test_main_index");

	hash_table_helper::truncate("test_main_index");

	{
		// Index full text
		full_text::index_single_batch("test_main_index", "test_main_index", "ALEXANDRIA-TEST-07");
	}

	{
		// Count elements in hash tables.
		hash_table::hash_table ht("test_main_index");

		// Make searches.
		full_text_index<full_text_record> index("test_main_index");

		const string query = "The latter is commonly used";
		struct search_metric metric;

		vector<full_text_record> results = search_engine::search_exact(allocation->record_storage, index, query, 1000, metric);

		BOOST_CHECK_EQUAL(results.size(), 1);
		BOOST_CHECK_EQUAL(metric.m_total_found, 1);
		BOOST_CHECK_EQUAL(metric.m_link_domain_matches, 0);
		BOOST_CHECK_EQUAL(metric.m_link_url_matches, 0);
		BOOST_CHECK_EQUAL(results[0].m_value, URL("https://en.wikipedia.org/wiki/A").hash());
	}

	search_allocation::delete_allocation(allocation);

	config::n_grams = 1;
	config::ft_max_results_per_section = initial_results_per_section;
}

BOOST_AUTO_TEST_SUITE_END()
