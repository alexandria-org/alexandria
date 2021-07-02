
#include "FullTextAdjustment.h"

FullTextAdjustment::FullTextAdjustment() {

}

FullTextAdjustment::~FullTextAdjustment() {

}

size_t FullTextAdjustment::count() const {
	return m_host_adjustments.size();
}

void FullTextAdjustment::add_host_adjustment(uint64_t word_hash, uint64_t source_url_hash, uint64_t target_host_hash,
	int harmonic) {

	m_host_adjustments[word_hash][source_url_hash] = make_pair(target_host_hash, harmonic);
}

unordered_map<uint64_t, unordered_map<uint64_t, vector<int>>> FullTextAdjustment::host_adjustments() const {

	unordered_map<uint64_t, unordered_map<uint64_t, vector<int>>> ret;
	for (auto &iter : m_host_adjustments) {
		for (auto &iter2 : iter.second) {
			ret[iter.first][iter2.second.first].push_back(iter2.second.second);
		}
	}

	return ret;

}

