
#pragma once

#include <iostream>
#include <sstream>
#include <fstream>
#include <set>
#include <vector>
#include <map>

using namespace std;

class TsvFile {

public:

	TsvFile(const string &file_name);
	~TsvFile();

	string find(const string &key);
	map<string, string> find_all(const set<string> &keys);

	size_t read_column_into(int column, set<string> &container);
	size_t read_column_into(int column, vector<string> &container);

private:

	ifstream m_file;
	size_t m_file_size;

	size_t binary_find_position(size_t file_size, size_t offset, const string &key);

};