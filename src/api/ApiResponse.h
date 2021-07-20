
#pragma once

#include <iostream>
#include <vector>

#include "api/ResultWithSnippet.h"

using namespace std;

class ApiResponse {

public:
	ApiResponse(vector<ResultWithSnippet> &results, size_t total_found, double profile);
	~ApiResponse();

	friend ostream &operator<<(ostream &os, const ApiResponse &api_response);

private:

	string m_response;

};
