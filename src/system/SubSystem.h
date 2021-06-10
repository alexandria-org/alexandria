
#pragma once

#include <aws/core/Aws.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/s3/S3Client.h>

#include "file/TsvFileS3.h"
#include "common/Dictionary.h"

class SubSystem {

public:

	SubSystem();
	~SubSystem();

	const Dictionary *domain_index() const;
	const Dictionary *dictionary() const;
	const Dictionary *full_text_dictionary() const;
	const vector<string> words() const;
	const Aws::S3::S3Client s3_client() const;

private:
	Dictionary *m_domain_index;
	Dictionary *m_dictionary;
	Dictionary *m_full_text_dictionary;
	Aws::S3::S3Client m_s3_client;
	vector<string> m_words;

};
