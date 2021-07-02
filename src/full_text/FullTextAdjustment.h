
#pragma once

#include <vector>
#include <unordered_map>
#include <cstdint>

using namespace std;

class FullTextAdjustment {

public:

	FullTextAdjustment();
	~FullTextAdjustment();

	size_t count() const;

	void add_host_adjustment(uint64_t word_hash, uint64_t source_url_hash, uint64_t target_host_hash, int harmonic);
	unordered_map<uint64_t, unordered_map<uint64_t, vector<int>>> host_adjustments() const;

private:

	unordered_map<uint64_t, unordered_map<uint64_t, pair<uint64_t, int>>> m_host_adjustments;

};
