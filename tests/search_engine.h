
#include "search_engine/SearchEngine.h"

BOOST_AUTO_TEST_SUITE(search_engine)

BOOST_AUTO_TEST_CASE(apply_link_scores) {

	vector<LinkFullTextRecord> links;
	vector<FullTextRecord> results;
	
	size_t links_applied;
	links_applied = SearchEngine::apply_link_scores(links, results);

	BOOST_CHECK_EQUAL(links_applied, 0);

	links = {LinkFullTextRecord{.m_value = 123, .m_score = 0.1, .m_target_hash = 10123}};
	results = {FullTextRecord{.m_value = 10123, .m_score = 0.1, .m_domain_hash = 9999}};

	float score_before = results[0].m_score;

	links_applied = SearchEngine::apply_link_scores(links, results);

	BOOST_CHECK_EQUAL(links_applied, 1);
	BOOST_CHECK(results[0].m_score > score_before);
	BOOST_REQUIRE_CLOSE(results[0].m_score, 0.1 + expm1(10*0.1) + 0.1, 0.0001);
}

BOOST_AUTO_TEST_CASE(apply_domain_link_scores) {

	vector<DomainLinkFullTextRecord> links;
	vector<FullTextRecord> results;
	
	size_t links_applied;
	links_applied = SearchEngine::apply_domain_link_scores(links, results);

	BOOST_CHECK_EQUAL(links_applied, 0);

	links = {DomainLinkFullTextRecord{.m_value = 123, .m_score = 0.1, .m_target_domain = 9999}};
	results = {FullTextRecord{.m_value = 10123, .m_score = 0.1, .m_domain_hash = 9999}};

	float score_before = results[0].m_score;

	links_applied = SearchEngine::apply_domain_link_scores(links, results);

	BOOST_CHECK_EQUAL(links_applied, 1);
	BOOST_CHECK(results[0].m_score > score_before);
	BOOST_REQUIRE_CLOSE(results[0].m_score, 0.1 + expm1(5*0.1) + 0.1, 0.0001);
}

BOOST_AUTO_TEST_SUITE_END();
