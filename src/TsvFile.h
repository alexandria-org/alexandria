
#pragma once

#include <iostream>
#include <fstream>

using namespace std;

class TsvFile {

public:

	TsvFile(const string &file_name);
	~TsvFile();

	string find(const string &key);

private:

	ifstream m_file;
	size_t m_file_size;

	size_t binary_find_position(size_t file_size, size_t offset, const string &key);

};