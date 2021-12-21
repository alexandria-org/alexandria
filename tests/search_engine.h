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

#include "search_engine/SearchEngine.h"

BOOST_AUTO_TEST_SUITE(search_engine)

BOOST_AUTO_TEST_CASE(apply_link_scores) {

	size_t links_applied;

	{
		vector<LinkFullTextRecord> links;
		FullTextResultSet<FullTextRecord> *results = new FullTextResultSet<FullTextRecord>(0);

		links_applied = SearchEngine::apply_link_scores(links, results);

		BOOST_CHECK_EQUAL(links_applied, 0);

		delete results;
	}

	vector<LinkFullTextRecord> links = {LinkFullTextRecord{.m_value = 123, .m_score = 0.1, .m_target_hash = 10123}};
	FullTextResultSet<FullTextRecord> *results = new FullTextResultSet<FullTextRecord>(1);
	FullTextRecord *data = results->data_pointer();
	data[0] = FullTextRecord{.m_value = 10123, .m_score = 0.1, .m_domain_hash = 9999};

	float score_before = data[0].m_score;

	links_applied = SearchEngine::apply_link_scores(links, results);

	BOOST_CHECK_EQUAL(links_applied, 1);
	BOOST_CHECK(data[0].m_score > score_before);
	BOOST_REQUIRE_CLOSE(data[0].m_score, 0.1 + expm1(25*0.1) / 50.0f, 0.0001);

	delete results;
}

BOOST_AUTO_TEST_CASE(apply_domain_link_scores) {

	size_t links_applied;
	{
		vector<DomainLink::FullTextRecord> links;
		FullTextResultSet<FullTextRecord> *results = new FullTextResultSet<FullTextRecord>(0);
		
		links_applied = SearchEngine::apply_domain_link_scores(links, results);

		BOOST_CHECK_EQUAL(links_applied, 0);

		delete results;
	}

	{
		vector<DomainLink::FullTextRecord> links = {DomainLink::FullTextRecord{.m_value = 123, .m_score = 0.1, .m_target_domain = 9999}};
		FullTextResultSet<FullTextRecord> *results = new FullTextResultSet<FullTextRecord>(1);

		float score_before = 0.1;
		FullTextRecord *data = results->data_pointer();
		data[0] = FullTextRecord{.m_value = 10123, .m_score = score_before, .m_domain_hash = 9999};

		links_applied = SearchEngine::apply_domain_link_scores(links, results);

		BOOST_CHECK_EQUAL(links_applied, 1);
		BOOST_CHECK(data[0].m_score > score_before);
		BOOST_REQUIRE_CLOSE(data[0].m_score, 0.1 + expm1(25*0.1) / 50.0f, 0.0001);

		delete results;
	}
}

BOOST_AUTO_TEST_SUITE_END()
