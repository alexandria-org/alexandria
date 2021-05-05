
#pragma once

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>

#include "TsvFileS3.h"
#include "Dictionary.h"

class SubSystem {

public:

	SubSystem(const Aws::S3::S3Client &s3_client);
	~SubSystem();

	const Dictionary *domain_index() const;
	const Dictionary *dictionary() const;
	const vector<string> words() const;
	const Aws::S3::S3Client s3_client() const;

private:
	Dictionary *m_domain_index;
	Dictionary *m_dictionary;
	Aws::S3::S3Client m_s3_client;
	vector<string> m_words;

};
