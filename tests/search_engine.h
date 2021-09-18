
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
		vector<DomainLinkFullTextRecord> links;
		FullTextResultSet<FullTextRecord> *results = new FullTextResultSet<FullTextRecord>(0);
		
		links_applied = SearchEngine::apply_domain_link_scores(links, results);

		BOOST_CHECK_EQUAL(links_applied, 0);

		delete results;
	}

	{
		vector<DomainLinkFullTextRecord> links = {DomainLinkFullTextRecord{.m_value = 123, .m_score = 0.1, .m_target_domain = 9999}};
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

BOOST_AUTO_TEST_SUITE_END();
