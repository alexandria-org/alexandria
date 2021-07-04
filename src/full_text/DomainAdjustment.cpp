
#include "DomainAdjustment.h"

DomainAdjustment::DomainAdjustment(const unordered_map<uint64_t, uint64_t> *url_to_domain)
: m_url_to_domain(url_to_domain)
{

}

DomainAdjustment::~DomainAdjustment() {

}

size_t DomainAdjustment::count() const {
	return m_host_adjustments.size();
}

void DomainAdjustment::add(uint64_t word_hash, const struct Link &link) {

	m_host_adjustments[word_hash][link.source_url.hash()] = link; 
}

unordered_map<uint64_t, unordered_map<uint64_t, vector<struct Link>>> DomainAdjustment::host_adjustments() const {

	unordered_map<uint64_t, unordered_map<uint64_t, vector<struct Link>>> ret;
	for (auto &iter : m_host_adjustments) {
		for (auto &iter2 : iter.second) {
			ret[iter.first][iter2.second.target_host_hash].push_back(iter2.second);
		}
	}

	return ret;

}

