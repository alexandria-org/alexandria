
#pragma once

#include <vector>
#include <unordered_map>
#include <cstdint>
#include "Adjustment.h"
#include "link_index/Link.h"

using namespace std;

class AdjustmentList {

public:
	size_t count() const;
	void add_domain_link(uint64_t word_hash, const Link &link);
	void add_link(uint64_t word_hash, const Link &link);
	vector<struct Adjustment> data() const;

private:

	unordered_map<uint64_t, unordered_map<uint64_t, Link>> m_domain_links;
	unordered_map<uint64_t, unordered_map<uint64_t, Link>> m_links;

};
