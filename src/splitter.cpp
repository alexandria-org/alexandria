
#include <iostream>
#include <vector>
#include "file/TsvFile.h"

using namespace std;

int main() {

	vector<string> batches = {
		"CC-MAIN-2021-31",
		"CC-MAIN-2021-25",
	};

	for (const string &batch : batches) {
		TsvFile warc_paths_file(string("crawl-data/") + batch + "/warc.paths.gz");

		vector<string> warc_paths;
		warc_paths_file.read_column_into(0, warc_paths);

		for (const string &file : warc_paths) {
			cout << file << endl;
		}
	}

	return 0;
}
