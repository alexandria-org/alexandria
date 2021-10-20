
#pragma once

#include <vector>
#include "SearchAllocation.h"
#include "full_text/FullTextShard.h"
#include "full_text/SearchMetric.h"
#include "link_index/LinkFullTextRecord.h"
#include "link_index/DomainLinkFullTextRecord.h"

template<typename DataRecord>
struct SearchArguments {
	string query;
	size_t limit;
	const vector<FullTextShard<DataRecord> *> *shards;
	struct SearchMetric *metric;
	const vector<LinkFullTextRecord> *links;
	const vector<DomainLinkFullTextRecord> *domain_links;
	SearchAllocation::Storage<DataRecord> *storage;
	size_t partition_id;
};
