
#pragma once

#include <iostream>
#include <sstream>
#include <fstream>
#include <set>
#include <vector>
#include <map>
#include <string.h>

using namespace std;

class TsvFile {

public:

	TsvFile();
	TsvFile(const string &file_name);
	~TsvFile();

	string find(const string &key);
	map<string, string> find_all(const set<string> &keys);

	size_t read_column_into(int column, set<string> &container);
	size_t read_column_into(int column, vector<string> &container);

	bool eof() const;
	string get_line();

protected:

	string m_file_name;
	string m_original_file_name;
	ifstream m_file;
	size_t m_file_size;
	bool m_is_gzipped = false;
	
	size_t binary_find_position(size_t file_size, size_t offset, const string &key);

	void set_file_name(const string &file_name);

};