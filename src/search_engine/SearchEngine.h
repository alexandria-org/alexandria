
#pragma once

#include <iostream>
#include <vector>
#include "full_text/FullTextRecord.h"
#include "full_text/FullTextShard.h"
#include "full_text/SearchMetric.h"
#include "link_index/LinkFullTextRecord.h"

using namespace std;

namespace SearchEngine {
	
	vector<FullTextRecord> search(const vector<FullTextShard<FullTextRecord> *> &shards, const vector<LinkFullTextRecord> &links,
		const string &query, size_t limit, struct SearchMetric &metric);

	vector<LinkFullTextRecord> search_links(const vector<FullTextShard<LinkFullTextRecord> *> &shards, const string &query, size_t limit,
		struct SearchMetric &metric);

}
