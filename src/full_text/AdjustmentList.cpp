
#include "AdjustmentList.h"

size_t AdjustmentList::count() const {
	return m_links.size() + m_domain_links.size();
}

void AdjustmentList::add_domain_link(uint64_t word_hash, const Link &link) {
	m_domain_links[word_hash][link.source_url().hash()] = link; 
}

void AdjustmentList::add_link(uint64_t word_hash, const Link &link) {
	m_links[word_hash][link.source_url().hash()] = link; 
}

vector<struct Adjustment> AdjustmentList::data() const {

	vector<struct Adjustment> ret;
	for (auto &iter : m_links) {
		for (auto &iter2 : iter.second) {
			ret.emplace_back(Adjustment{
				.type = URL_ADJUSTMENT,
				.word_hash = iter.first,
				.key_hash = iter2.second.target_url().hash(),
				.score = iter2.second.url_score(),
				.domain_harmonic = (uint32_t)iter2.second.target_harmonic()
			});
		}
	}

	for (auto &iter : m_domain_links) {
		for (auto &iter2 : iter.second) {
			ret.emplace_back(Adjustment{
				.type = DOMAIN_ADJUSTMENT,
				.word_hash = iter.first,
				.key_hash = iter2.second.target_host_hash(),
				.score = iter2.second.domain_score(),
				.domain_harmonic = (uint32_t)iter2.second.target_harmonic()
			});
		}
	}

	return ret;

}

