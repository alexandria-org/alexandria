
#pragma once

#include <vector>
#include <unordered_map>
#include <cstdint>
#include "link_index/Link.h"

using namespace std;

class URLAdjustment {

public:

	URLAdjustment();
	~URLAdjustment();

	size_t count() const;

	void add(uint64_t word_hash, const struct Link &link);
	unordered_map<uint64_t, unordered_map<uint64_t, vector<struct Link>>> url_adjustments() const;

private:

	unordered_map<uint64_t, unordered_map<uint64_t, struct Link>> m_url_adjustments;

};
