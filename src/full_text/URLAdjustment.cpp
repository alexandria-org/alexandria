
#include "URLAdjustment.h"

URLAdjustment::URLAdjustment() {

}

URLAdjustment::~URLAdjustment() {

}

size_t URLAdjustment::count() const {
	return m_url_adjustments.size();
}

void URLAdjustment::add(uint64_t word_hash, const struct Link &link) {

	m_url_adjustments[word_hash][link.source_url.hash()] = link; 
}

unordered_map<uint64_t, unordered_map<uint64_t, vector<struct Link>>> URLAdjustment::url_adjustments() const {

	unordered_map<uint64_t, unordered_map<uint64_t, vector<struct Link>>> ret;
	for (auto &iter : m_url_adjustments) {
		for (auto &iter2 : iter.second) {
			ret[iter.first][iter2.second.target_url.hash()].push_back(iter2.second);
		}
	}

	return ret;

}

