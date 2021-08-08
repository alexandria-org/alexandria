
#pragma once

#include <iostream>
#include <fstream>

using namespace std;

namespace File {

	string read_test_file(const string &file_name) {
		ifstream file("../tests/data/" + file_name);
		if (file.is_open()) {
			string ret;
			file.seekg(0, ios::end);
			ret.resize(file.tellg());
			file.seekg(0, ios::beg);
			file.read(&ret[0], ret.size());
			file.close();
			return ret;
		}
		return "";
	}

}
