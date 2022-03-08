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
#include "indexer/index_builder.h"
#include "indexer/index.h"
#include "indexer/sharded_index_builder.h"
#include "indexer/sharded_index.h"
#include "indexer/snippet.h"
#include "indexer/index_tree.h"
#include "algorithm/HyperLogLog.h"
#include "parser/URL.h"
#include "transfer/Transfer.h"
#include "memory/debugger.h"

BOOST_AUTO_TEST_SUITE(index_array)

BOOST_AUTO_TEST_CASE(index_builder) {

	memory::enable_debugger();
	{
		// Max 10 results in this index.
		indexer::index_builder<indexer::generic_record> idx("test", 0, 1000, 10);
		idx.truncate();

		Algorithm::HyperLogLog<size_t> ss;
		for (size_t i = 1; i <= 100; i++) {
			ss.insert(i);
		}

		BOOST_CHECK(ss.size() == 100);

		for (size_t i = 1; i <= 100; i++) {
			idx.add(123, indexer::generic_record(i, 0.1f * i));
			if (i == 50) {
				idx.append();
				idx.merge();
			}
		}

		idx.append();
		idx.merge();
	}
	BOOST_CHECK_EQUAL(memory::disable_debugger(), 0);

	memory::enable_debugger();
	{
		indexer::index<indexer::generic_record> idx("test", 0, 1000);
		size_t total;
		std::vector<indexer::generic_record> res = idx.find(123, total);
		// Results are sorted by value.
		BOOST_REQUIRE_EQUAL(res.size(), 10);
		BOOST_CHECK_EQUAL(total, 100);

		std::sort(res.begin(), res.end(), [](const indexer::generic_record &a, const indexer::generic_record &b) {
			return a.m_score > b.m_score;
		});
		BOOST_CHECK_EQUAL(res[0].m_value, 100);
	}
	BOOST_CHECK_EQUAL(memory::disable_debugger(), 0);

}

BOOST_AUTO_TEST_CASE(index) {

	/*
	 * This is the simplest form. We can create an index and add records to it. Then search for the keys.
	 * */
	{
		indexer::index_builder<indexer::generic_record> idx("test", 0);
		idx.truncate();

		idx.add(123, indexer::generic_record(1, 0.2f));
		idx.add(123, indexer::generic_record(2, 0.3f));

		idx.append();
		idx.merge();
	}

	{
		indexer::index<indexer::generic_record> idx("test", 0);
		std::vector<indexer::generic_record> res = idx.find(123);
		// Results are sorted by value.
		BOOST_CHECK_EQUAL(res[0].m_value, 1);
		BOOST_CHECK_EQUAL(res[1].m_value, 2);
	}

}

BOOST_AUTO_TEST_CASE(sharded_index) {

	struct record {

		uint64_t m_value;
		float m_score;

	};

	/*
	 * This is the simplest form. We can create an index and add records to it. Then search for the keys.
	 * */
	{
		indexer::sharded_index_builder<indexer::generic_record> idx("sharded_index", 10);
		idx.truncate();

		for (size_t i = 0; i < 1000; i++) {
			idx.add(i, indexer::generic_record(i, 0.2f));
		}

		idx.append();
		idx.merge();
	}

	{
		indexer::sharded_index<indexer::generic_record> idx("sharded_index", 10);
		std::vector<indexer::generic_record> res = idx.find(123);
		// Results are sorted by value.
		BOOST_REQUIRE_EQUAL(res.size(), 1);
		BOOST_CHECK_EQUAL(res[0].m_value, 123);
	}

}

BOOST_AUTO_TEST_CASE(index_frequency) {

	struct record {

		uint64_t m_value;
		float m_score;

	};

	/*
	 * This is the simplest form. We can create an index and add records to it. Then search for the keys.
	 * */
	{
		indexer::index_builder<indexer::domain_record> idx("test", 0);
		idx.truncate();

		// Document 1
		idx.add(123, indexer::domain_record(1, 0.2f));

		// Document 2
		idx.add(123, indexer::domain_record(2, 0.3f));
		idx.add(123, indexer::domain_record(2, 0.3f));
		idx.add(111, indexer::domain_record(2, 0.3f));
		idx.add(112, indexer::domain_record(2, 0.3f));

		// Document 3
		idx.add(113, indexer::domain_record(3, 0.3f));

		idx.append();
		idx.merge();

		BOOST_CHECK_EQUAL(idx.document_size(1), 1);
		BOOST_CHECK_EQUAL(idx.document_size(2), 4);
		BOOST_CHECK_EQUAL(idx.document_size(3), 1);

		idx.calculate_scores(indexer::algorithm::tf_idf);
	}

	{
		indexer::index<indexer::domain_record> idx("test", 0);
		size_t total = 0;
		idx.find(113, total);

		BOOST_CHECK(idx.get_document_count() == 3);
		BOOST_CHECK(total == 1);

		std::vector<indexer::domain_record> res = idx.find(123, total);
		BOOST_CHECK(total == 2);
		BOOST_REQUIRE(res.size() == 2);
		BOOST_CHECK(res[0].m_value == 1);
		BOOST_CHECK_EQUAL(res[0].m_score, res[1].m_score * 2);
	}

}

BOOST_AUTO_TEST_CASE(index_frequency_2) {

	indexer::index_tree idx_tree;

	indexer::domain_level domain_level;

	idx_tree.add_level(&domain_level);

	idx_tree.truncate();

	// Testing example from here: https://remykarem.github.io/tfidf-demo/
	idx_tree.add_document(1, "Air quality in the sunny island improved gradually throughout Wednesday.");
	idx_tree.add_document(2, "Air quality in Singapore on Wednesday continued to get worse as haze hit the island.");
	idx_tree.add_document(3, "The air quality in Singapore is monitored through a network of air monitoring stations located in different parts of the island");
	idx_tree.add_document(4, "The air quality in Singapore got worse on Wednesday.");

	idx_tree.merge();
	idx_tree.calculate_scores_for_level(0);

	{
		std::vector<indexer::return_record> res = idx_tree.find("Air");

		BOOST_REQUIRE(res.size() == 4);
		BOOST_CHECK(res[0].m_value == 4);
		BOOST_CHECK(res[1].m_value == 1);
		BOOST_CHECK(res[2].m_value == 3);
		BOOST_CHECK(res[3].m_value == 2);
	}

	{
		std::vector<indexer::return_record> res = idx_tree.find("quality");

		BOOST_REQUIRE(res.size() == 4);
		BOOST_CHECK(res[0].m_value == 4);
		BOOST_CHECK(res[1].m_value == 1);
		BOOST_CHECK(res[2].m_value == 2);
		BOOST_CHECK(res[3].m_value == 3);	
	}

	{
		std::vector<indexer::return_record> res = idx_tree.find("wednesday");

		BOOST_REQUIRE(res.size() == 3);
		BOOST_CHECK(res[0].m_value == 4);
		BOOST_CHECK(res[1].m_value == 1);
		BOOST_CHECK(res[2].m_value == 2);	
	}

	{
		std::vector<indexer::return_record> res = idx_tree.find("wednesday");

		BOOST_REQUIRE(res.size() == 3);
		BOOST_CHECK(res[0].m_value == 4);
		BOOST_CHECK(res[1].m_value == 1);
		BOOST_CHECK(res[2].m_value == 2);	
	}
}

BOOST_AUTO_TEST_CASE(snippet) {

	indexer::snippet snippet("mukandengineers.com", "http://mukandengineers.com/", 0, "Employing more than 200");
	auto tokens = snippet.tokens();

	BOOST_REQUIRE_EQUAL(tokens.size(), 4);
	BOOST_CHECK_EQUAL(tokens[0], Hash::str("employing"));
	BOOST_CHECK_EQUAL(tokens[1], Hash::str("more"));
	BOOST_CHECK_EQUAL(tokens[2], Hash::str("than"));
	BOOST_CHECK_EQUAL(tokens[3], Hash::str("200"));

	BOOST_CHECK_EQUAL(snippet.domain_hash(), Hash::str("mukandengineers.com"));
	BOOST_CHECK_EQUAL(snippet.url_hash(), URL("http://mukandengineers.com/").hash());
}

BOOST_AUTO_TEST_CASE(index_tree) {

	{
		indexer::index_tree idx_tree;

		indexer::domain_level domain_level;
		indexer::url_level url_level;
		indexer::snippet_level snippet_level;

		idx_tree.add_level(&domain_level);
		idx_tree.add_level(&url_level);
		idx_tree.add_level(&snippet_level);

		idx_tree.truncate();

		indexer::snippet snippet("mukandengineers.com", "http://mukandengineers.com/", 0, "Employing more than 200 engineers, the company has undertaken several challenging and prestigious projects across many industries in India and is today known for its skill and reliability in delivering quality services. The company is equipped with a whole range of construction machinery  including mobile cranes, gantry cranes, welding machines, concrete batching plants, transit mixers , electrical test and measuring instruments.");

		idx_tree.add_snippet(snippet);
		idx_tree.merge();

		std::vector<indexer::return_record> res = idx_tree.find("Employing more than");

		BOOST_REQUIRE_EQUAL(res.size(), 1);
		BOOST_CHECK_EQUAL(res[0].m_value, snippet.snippet_hash());
	}

}

BOOST_AUTO_TEST_CASE(index_tree2) {

	memory::enable_debugger();
	{
		indexer::index_tree idx_tree;

		indexer::domain_level domain_level;
		indexer::url_level url_level;
		indexer::snippet_level snippet_level;

		idx_tree.add_level(&domain_level);
		idx_tree.add_level(&url_level);
		idx_tree.add_level(&snippet_level);

		idx_tree.truncate();

		indexer::snippet snippet1("example.com", "http://example.com/url1", 0, "This is an example page");
		indexer::snippet snippet2("example.com", "http://example.com/url2", 0, "This is an example page 2");
		indexer::snippet snippet3("example.com", "http://example.com/url3", 0, "This is an example page 3");
		indexer::snippet snippet4("example.com", "http://example.com/url3", 1, "example page");

		idx_tree.add_snippet(snippet1);
		idx_tree.add_snippet(snippet2);
		idx_tree.add_snippet(snippet3);
		idx_tree.add_snippet(snippet4);
		idx_tree.merge();

		std::vector<indexer::return_record> res = idx_tree.find("example page 2");

		BOOST_REQUIRE_EQUAL(res.size(), 1);
		BOOST_CHECK_EQUAL(res[0].m_value, snippet2.snippet_hash());

		std::vector<indexer::return_record> res2 = idx_tree.find("example page");

		BOOST_REQUIRE_EQUAL(res2.size(), 4);
	}
	BOOST_CHECK_EQUAL(memory::disable_debugger(), 0);

}

BOOST_AUTO_TEST_CASE(index_files) {
/*
	{
		indexer::index_tree idx_tree;

		indexer::domain_level domain_level;
		indexer::url_level url_level;
		indexer::snippet_level snippet_level;

		idx_tree.add_level(&domain_level);
		idx_tree.add_level(&url_level);
		idx_tree.add_level(&snippet_level);

		idx_tree.truncate();

		std::vector<std::string> local_files = Transfer::download_gz_files_to_disk(
			{std::string("crawl-data/ALEXANDRIA-TEST-10/test00.gz")}
		);
		idx_tree.add_index_file(local_files[0]);
		Transfer::delete_downloaded_files(local_files);

		idx_tree.merge();

		std::vector<indexer::return_record> res = idx_tree.find("main characters are Sloane Margaret Jameson");

		BOOST_REQUIRE_EQUAL(res.size(), 2);

		HashTable ht("index_tree");
		const std::string snippet1 = ht.find(res[0].m_value);
		std::cout << "snippet1: " << snippet1 << std::endl;

		const std::string snippet2 = ht.find(res[1].m_value);
		std::cout << "snippet2: " << snippet2 << std::endl;
		//BOOST_CHECK_EQUAL(res[0].m_value, snippet.snippet_hash());
	}*/

}

BOOST_AUTO_TEST_SUITE_END()
