
#pragma once

#include <iostream>
#include <vector>
#include "api/ResultWithSnippet.h"

using namespace std;

class PostProcessor {

public:
	PostProcessor(const string &query);
	~PostProcessor();

	void run(vector<ResultWithSnippet> &results);

private:


};
