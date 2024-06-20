
#pragma once

#include "URL.h"
#include "generic_record.h"
#include "text/text.h"

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
		URL m_url;
		std::string m_title;
		std::string m_snippet;
		std::string m_meta;

		return_record() : generic_record() {};
		return_record(uint64_t value) : generic_record(value) {};
		return_record(uint64_t value, float score) : generic_record(value, score) {};
		return_record(uint64_t value, float score, const std::string &tsv_data) : generic_record(value, score) {

			size_t pos_start = 0;
			size_t pos_end = 0;
			size_t col_num = 0;
			while (pos_end != std::string::npos) {
				pos_end = tsv_data.find('\t', pos_start);
				const size_t len = pos_end - pos_start;
				if (col_num == 0) {
					m_url = URL(tsv_data.substr(pos_start, len));
				}
				if (col_num == 1) {
					m_title = tsv_data.substr(pos_start, len);
				}
				if (col_num == 3) {
					m_meta = tsv_data.substr(pos_start, len);
				}
				if (col_num == 4) {
					m_snippet = make_snippet(tsv_data.substr(pos_start, len));
					if (m_snippet.size() == 0) {
						m_snippet = make_snippet(m_meta);
					}
				}

				pos_start = pos_end + 1;
				col_num++;
			}

		};

		private:
		std::string make_snippet(const std::string &text) const {
			auto response = text.substr(0, 140);
			text::trim(response);
			if (response.size() >= 140) response += "...";
			return response;
		}

	};
}
