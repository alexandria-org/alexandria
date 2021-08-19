
#pragma once

#include <iostream>
#include <vector>
#include "full_text/FullTextIndex.h"
#include "full_text/FullTextRecord.h"
#include "full_text/FullTextShard.h"
#include "full_text/SearchMetric.h"
#include "link_index/LinkFullTextRecord.h"
#include "link_index/DomainLinkFullTextRecord.h"

using namespace std;

namespace SearchEngine {
	
	vector<FullTextRecord> search(const vector<FullTextShard<FullTextRecord> *> &shards, const vector<LinkFullTextRecord> &links,
		const string &query, size_t limit, struct SearchMetric &metric);

	vector<FullTextRecord> search_with_domain_links(const vector<FullTextShard<FullTextRecord> *> &shards, const vector<LinkFullTextRecord> &links,
		const vector<DomainLinkFullTextRecord> &domain_links, const string &query, size_t limit, struct SearchMetric &metric);

	vector<LinkFullTextRecord> search_links(const vector<FullTextShard<LinkFullTextRecord> *> &shards, const string &query, struct SearchMetric &metric);

	vector<FullTextRecord> search_index_array(vector<FullTextIndex<FullTextRecord> *> index_array, const vector<LinkFullTextRecord> &links,
		const string &query, size_t limit, struct SearchMetric &metric);

	vector<FullTextRecord> search_index_array(vector<FullTextIndex<FullTextRecord> *> index_array, const vector<LinkFullTextRecord> &links,
		const vector<DomainLinkFullTextRecord> &domain_links, const string &query, size_t limit, struct SearchMetric &metric);

	vector<LinkFullTextRecord> search_link_array(vector<FullTextIndex<LinkFullTextRecord> *> index_array, const string &query, size_t limit,
		struct SearchMetric &metric);

	vector<DomainLinkFullTextRecord> search_domain_link_array(vector<FullTextIndex<DomainLinkFullTextRecord> *> index_array, const string &query,
		size_t limit, struct SearchMetric &metric);

	/*
		Add scores for the given links to the result set. The links are assumed to be ordered by link.m_target_hash ascending.
	*/
	size_t apply_link_scores(const vector<LinkFullTextRecord> &links, vector<FullTextRecord> &results);
	size_t apply_domain_link_scores(const vector<DomainLinkFullTextRecord> &links, vector<FullTextRecord> &results);

}
