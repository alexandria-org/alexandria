
#include "SubSystem.h"

SubSystem::SubSystem(const Aws::S3::S3Client &s3_client) {

	TsvFileS3 domain_index(s3_client, "domain_info.tsv");
	m_domain_index = new Dictionary(domain_index);

	TsvFileS3 dictionary(s3_client, "dictionary.tsv");
	m_dictionary = new Dictionary(dictionary);

	m_s3_client = s3_client;

}

SubSystem::~SubSystem() {
	delete m_dictionary;
	delete m_domain_index;
}

const Dictionary *SubSystem::domain_index() const {
	return m_domain_index;
}

const Dictionary *SubSystem::dictionary() const {
	return m_dictionary;
}

const Aws::S3::S3Client SubSystem::s3_client() const {
	return m_s3_client;
}

