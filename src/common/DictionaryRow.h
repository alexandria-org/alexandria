
#pragma once

#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

#define CC_ROW_LEN 5

class DictionaryRow {

public:

	DictionaryRow();
	DictionaryRow(const DictionaryRow &row);
	DictionaryRow(const string &row);
	DictionaryRow(stringstream &stream);
	~DictionaryRow();

	int get_int(int column) const;
	float get_float(int column) const;
	double get_double(int column) const;

private:
	vector<double> m_columns;

	void read_stream(stringstream &stream);

};
