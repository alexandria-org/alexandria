
#include "UrlToDomain.h"
#include "system/Logger.h"

UrlToDomain::UrlToDomain(const string &db_name)
: m_db_name(db_name)
{
	
}

UrlToDomain::~UrlToDomain() {

}

void UrlToDomain::add_url(uint64_t url_hash, uint64_t domain_hash) {
	m_url_to_domain[url_hash] = domain_hash;
}

void UrlToDomain::read() {
	for (size_t bucket_id = 0; bucket_id < 8; bucket_id++) {
		const string file_name = "/mnt/"+(to_string(bucket_id))+"/full_text/url_to_domain_"+m_db_name+".fti";

		ifstream infile(file_name, ios::binary);
		if (infile.is_open()) {

			char buffer[8];

			do {

				infile.read(buffer, sizeof(uint64_t));
				if (infile.eof()) break;

				uint64_t url_hash = *((uint64_t *)buffer);

				infile.read(buffer, sizeof(uint64_t));
				uint64_t domain_hash = *((uint64_t *)buffer);

				m_url_to_domain[url_hash] = domain_hash;
				m_domains[domain_hash]++;

			} while (!infile.eof());

			infile.close();
		}
	}
}

void UrlToDomain::write(size_t indexer_id) {
	const string file_name = "/mnt/"+(to_string(indexer_id % 8))+"/full_text/url_to_domain_"+m_db_name+".fti";

	ofstream outfile(file_name, ios::binary | ios::app);
	if (!outfile.is_open()) {
		throw error("Could not open url_to_domain file");
	}

	for (const auto &iter : m_url_to_domain) {
		outfile.write((const char *)&(iter.first), sizeof(uint64_t));
		outfile.write((const char *)&(iter.second), sizeof(uint64_t));
	}

	outfile.close();
}

