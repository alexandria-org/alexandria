
#pragma once

#include "generic_record.h"

namespace indexer {

	/*
	This is the returned record from the index_manager. It contains more data than the stored record.
	*/
	class return_record : public generic_record {

		public:
		uint64_t m_url_hash;
		uint64_t m_domain_hash;
		size_t m_num_url_links = 0;
		size_t m_num_domain_links = 0;

		return_record() : generic_record() {};
		return_record(uint64_t value) : generic_record(value) {};
		return_record(uint64_t value, float score) : generic_record(value, score) {};

	};
}
