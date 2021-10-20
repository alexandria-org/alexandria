
#include <cstdlib>
#include <chrono>
#include <thread>
#include <cstring>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <span>

using namespace std;

void *malloc_on_each(void *) {
	for (size_t i = 0; i < 200; i++) {
		size_t len = 50000000;
		char *buffer = new char[len];
		memset(buffer, 0, len);
		delete [] buffer;
	}
	return NULL;
}

void *malloc_once(void *) {
	size_t len = 50000000;
	char *buffer = new char[len];
	for (size_t i = 0; i < 200; i++) {
		memset(buffer, 0, len);
	}
	delete [] buffer;
	return NULL;
}

struct SStruct {
	size_t a;
	size_t b;
	size_t c;
};

void *openfile(void *) {
	//ifstream infile("/mnt/3/full_text/url_to_domain_main_index.fti", ios::in);
	ifstream infile("/root/url_to_domain_main_index.fti", ios::in);
	infile.seekg(0, ios::end);
	const size_t filesize = infile.tellg();

	SStruct *buffer = new SStruct[71838804];
	infile.read((char *)buffer, filesize);
	infile.close();
	delete buffer;

	return NULL;
}  

int main() {

	const size_t sz = 20000000;
	SStruct *buffer = new SStruct[sz];
	for (size_t i = 0; i < sz; i++) {
		buffer[i].a = rand();
	}

	span<SStruct> sp(buffer, sz);

	std::chrono::_V2::system_clock::time_point start_time = std::chrono::high_resolution_clock::now();
	sort(sp.begin(), sp.end(), [](const struct SStruct &a, const struct SStruct &b) { return a.a < b.a; });

	auto timer_elapsed = chrono::high_resolution_clock::now() - start_time;
	auto microseconds = chrono::duration_cast<std::chrono::microseconds>(timer_elapsed).count();

	cout << "sort took: " << microseconds/1000 << " milliseconds\n";

	delete buffer;

	return 0;

	const size_t num_threads = 30;
	pthread_t thread_ids[num_threads];
	for (size_t i = 0; i < num_threads; i++) {
		pthread_create(&thread_ids[i], NULL, openfile, NULL);
	}
	for (size_t i = 0; i < num_threads; i++) {
		pthread_join(thread_ids[i], NULL);
	}

	return 0;
}
