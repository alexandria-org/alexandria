
#pragma once

#include <vector>
#include <unordered_map>
#include <cstdint>
#include "link_index/Link.h"

using namespace std;

class DomainAdjustment {

public:

	DomainAdjustment(const unordered_map<uint64_t, uint64_t> *url_to_domain);
	~DomainAdjustment();

	size_t count() const;
	const unordered_map<uint64_t, uint64_t> *url_to_domain() const { return m_url_to_domain; };

	void add(uint64_t word_hash, const struct Link &link);
	unordered_map<uint64_t, unordered_map<uint64_t, vector<struct Link>>> host_adjustments() const;

private:

	const unordered_map<uint64_t, uint64_t> *m_url_to_domain;
	unordered_map<uint64_t, unordered_map<uint64_t, struct Link>> m_host_adjustments;

};
