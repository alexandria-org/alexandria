
#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/core/utils/json/JsonSerializer.h>

#include "SearchResult.h"
#include "TextBase.h"

using namespace std;

class ApiResponse : public TextBase {

public:
	ApiResponse();
	~ApiResponse();

	void set_results(vector<SearchResult> &results);
	void set_debug(const string &variable_name, size_t value);
	void set_failure(const string &reason);

	vector<SearchResult> results() const;
	string status() const;
	string json() const;

private:

	vector<SearchResult> m_results;
	map<string, size_t> m_debug_variables;
	string m_status;
	string m_failure_reason;

};