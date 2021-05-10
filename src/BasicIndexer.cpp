
#include "BasicIndexer.h"

void BasicIndexer::index(const vector<string> &file_names, int shard) {

	map<string, string> index;
	size_t bytes_read = 0;
	const size_t max_bytes_read = (1024ul*1024ul*1024ul*10ul); // 10 gb
	for (const string &file_name : file_names) {
		ifstream infile(file_name);
		string line;
		while (getline(infile, line)) {
			const string word = line.substr(0, line.find("\t"));
			index[word] += line + "\n";
			bytes_read += line.size() + 1;
		}
		infile.close();

		if (bytes_read > max_bytes_read) {
			// Flush.
			for (const auto &iter : index) {
				if (iter.second.size()) {
					ofstream outfile("/mnt/" + to_string(shard) + "/output/index_" + iter.first + ".tsv", ios::app);
					if (outfile.is_open()) {
						outfile << iter.second;
						outfile.close();
					}
				}
			}
			index.clear();
			bytes_read = 0;
		}
	}
	// Flush.
	for (const auto &iter : index) {
		if (iter.second.size()) {
			ofstream outfile("/mnt/" + to_string(shard) + "/output/index_" + iter.first + ".tsv", ios::app);
			if (outfile.is_open()) {
				outfile << iter.second;
				outfile.close();
			}
		}
	}

}