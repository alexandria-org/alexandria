
#include <iostream>
#include <fstream>
#include <vector>

#include "system/Profiler.h"

using namespace std;

int main() {

	/*
	ofstream file("/mnt/0/test.dat", ios::binary | ios::trunc);
	size_t bytes_written = 0;
	for (size_t i = 0; i < 100000000; i++) {
		uint64_t rnd1 = i;
		float score = 123;
		file.write((const char *)&rnd1, sizeof(rnd1));
		file.write((const char *)&score, sizeof(score));
		bytes_written += sizeof(rnd1) + sizeof(score);
	}
	file.close();

	cout << "bytes_written: " << bytes_written << endl;
	
	return 0;*/
	Profiler pf("Total");
	ifstream file("/mnt/0/test.dat", ios::binary | ios::ate);
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(size);
	if (file.read(buffer.data(), size)) {
		for (size_t i = 0; i < 10000000; i++) {
			const uint64_t *rnd1 = (const uint64_t *)&buffer[i * 12];
			const float *score = (const float *)&buffer[i * 12 + 8];
			//cout << *rnd1 << " " << *score << endl;
		}
	}
}
