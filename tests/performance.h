
#include "sort/Sort.h"
#include "system/Profiler.h"

BOOST_AUTO_TEST_SUITE(performance)

BOOST_AUTO_TEST_CASE(domain_index_sharp) {

	struct SearchMetric metric;
	string query = "facebook";

	vector<FullTextIndex<DomainLinkFullTextRecord> *> domain_link_index_array =
		FullText::create_index_array<DomainLinkFullTextRecord>("domain_link_index", 8);
	
	Profiler::instance profiler_domain_links("SearchEngine::search_domain_link_array");
	vector<DomainLinkFullTextRecord> domain_links = SearchEngine::search<DomainLinkFullTextRecord>(domain_link_index_array, query, 10000, metric);
	profiler_domain_links.stop();

	
}

BOOST_AUTO_TEST_CASE(domain_index) {

	vector<FullTextShard<DomainLinkFullTextRecord> *> shards;

	shards.push_back(new FullTextShard<DomainLinkFullTextRecord>("domain_link_index_0", 859));
	shards.push_back(new FullTextShard<DomainLinkFullTextRecord>("domain_link_index_1", 859));
	shards.push_back(new FullTextShard<DomainLinkFullTextRecord>("domain_link_index_2", 859));
	shards.push_back(new FullTextShard<DomainLinkFullTextRecord>("domain_link_index_3", 859));
	shards.push_back(new FullTextShard<DomainLinkFullTextRecord>("domain_link_index_4", 859));
	shards.push_back(new FullTextShard<DomainLinkFullTextRecord>("domain_link_index_5", 859));
	shards.push_back(new FullTextShard<DomainLinkFullTextRecord>("domain_link_index_6", 859));
	shards.push_back(new FullTextShard<DomainLinkFullTextRecord>("domain_link_index_7", 859));

	const uint64_t key = 10850050818246762331ull;

	Profiler::measure_base_performance();

	//BOOST_CHECK(num_cycles < 700000000);
}

BOOST_AUTO_TEST_SUITE_END();
