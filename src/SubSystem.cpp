
#include "SubSystem.h"

SubSystem::SubSystem(const Aws::S3::S3Client &s3_client) {

	TsvFileS3 domain_index(s3_client, "domain_info.tsv");
	m_domain_index = new Dictionary(domain_index);

	TsvFileS3 dictionary(s3_client, "dictionary.tsv");

	m_dictionary = new Dictionary(dictionary);

	dictionary.read_column_into(0, m_words);

	sort(m_words.begin(), m_words.end(), [](const string &a, const string &b) {
		return a < b;
	});

	m_s3_client = s3_client;

	//TsvFileS3 dictionary(s3_client, "full_text_dictionary.tsv");
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

const vector<string> SubSystem::words() const {
	return m_words;
}

const Aws::S3::S3Client SubSystem::s3_client() const {
	return m_s3_client;
}

