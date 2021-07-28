
#pragma once

#include <iostream>
#include <vector>

#include "api/ResultWithSnippet.h"
#include "full_text/SearchMetric.h"

using namespace std;

class ApiResponse {

public:
	ApiResponse(vector<ResultWithSnippet> &results, const struct SearchMetric &metric, double profile);
	~ApiResponse();

	friend ostream &operator<<(ostream &os, const ApiResponse &api_response);

private:

	string m_response;

};
