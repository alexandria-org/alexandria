
#pragma once

#include <iostream>
#include <fstream>
#include <stdio.h>

using namespace std;

namespace File {

	string read_test_file(const string &file_name);
	void copy_file(const string &source, const string &dest);
	void delete_file(const string &file);

}
