
#pragma once

#include <iostream>
#include <vector>

using namespace std;

class TsvRow {

public:
	TsvRow(const string &line);
	~TsvRow();

private:
	vector<string> m_cols;

};