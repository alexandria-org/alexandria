/*
 * MIT License
 *
 * Alexandria.org
 *
 * Copyright (c) 2021 Josef Cullhed, <info@alexandria.org>, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <iomanip>
#include <aws/core/Aws.h>
#include <aws/lambda/LambdaClient.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/s3/S3Client.h>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/algorithm/string.hpp>

#include "file/TsvFileS3.h"
#include "file/TsvFileRemote.h"
#include "common/Dictionary.h"

using namespace boost::iostreams;
using namespace std;

class SubSystem {

public:

	SubSystem();
	~SubSystem();

	const Dictionary *domain_index() const;
	const Dictionary *dictionary() const;
	const Dictionary *full_text_dictionary() const;
	const vector<string> words() const;
	const Aws::S3::S3Client s3_client() const;
	const Aws::Lambda::LambdaClient lambda_client() const;

private:
	Dictionary *m_domain_index;
	Dictionary *m_dictionary;
	Dictionary *m_full_text_dictionary;
	Aws::S3::S3Client *m_s3_client;
	Aws::Lambda::LambdaClient *m_lambda_client;
	vector<string> m_words;

	void init_aws_api();
	void deinit_aws_api();
	Aws::Client::ClientConfiguration get_s3_config();

};
