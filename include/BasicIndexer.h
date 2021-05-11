
#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <fstream>

using namespace std;

class BasicIndexer {

public:

	void index(const vector<string> &file_names, int shard);

};