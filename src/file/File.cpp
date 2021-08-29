
#include "File.h"

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

	void copy_file(const string &source, const string &dest) {
		ifstream infile(source, ios::binary);
		ofstream outfile(dest, ios::binary | ios::trunc);

		outfile << infile.rdbuf();
	}

	void delete_file(const string &file) {
		remove(file.c_str());
	}
}
