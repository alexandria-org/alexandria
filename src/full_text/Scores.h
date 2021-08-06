
#pragma once

#include <iostream>
#include <map>
#include <cstdint>
#include "parser/URL.h"
#include "text/Text.h"
#include "SearchMetric.h"
#include "FullTextRecord.h"
#include "link_index/LinkFullTextRecord.h"

using namespace std;

namespace Scores {

	/*
		Add scores for the given links to the result set. The links are assumed to be ordered by link.m_target_hash ascending.
	*/
	void apply_link_scores(const vector<LinkFullTextRecord> &links, vector<FullTextRecord> &results, struct SearchMetric &metric);

}
