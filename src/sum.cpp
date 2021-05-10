
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>

using namespace std;

int main() {

	ifstream infile("/host/alexandria.org/dictionary.tsv");
	string line;
	map<string, int> word_counts;
	int errors = 0;
	while (getline(infile, line)) {
		if (line == "") {
			errors++;
			cout << "Error 1" << endl;
			continue;
		}
		size_t delim = line.find("\t");
		if (delim == string::npos) {
			errors++;
			cout << "Error 2" << endl;
			continue;
		}
		const string word = line.substr(0, delim);
		const string count_str = line.substr(delim + 1);

		try {
			int count = stoi(count_str);
			word_counts[word] += count;
		} catch (...) {
			cout << "Error 3" << endl;
			errors++;
		}
	}

	cout << "Errors: " << errors << endl;

	typedef pair<string, int> pair_t;
	vector<pair_t> vec;

	copy(word_counts.begin(), word_counts.end(), std::back_inserter(vec));

	std::sort(vec.begin(), vec.end(), [](const pair_t& l, const pair_t& r) {
		if (l.second != r.second) {
			return l.second > r.second;
		}

		return l.first > r.first;
	});

	string result;
	string full_text_index;
	uint64_t word_idx = 1ul;
	for (const auto &iter : vec) {
		result += iter.first + "\t" + to_string(iter.second) + "\n";
		full_text_index += iter.first + "\t" + to_string(word_idx) + "\n";
		word_idx++;
	}

	ofstream outfile;
	outfile.open("/host/alexandria.org/main_dictionary.tsv", ios::app);
	outfile << result;
	outfile.close();

	ofstream outfile2;
	outfile2.open("/host/alexandria.org/full_text_index.tsv", ios::app);
	outfile2 << full_text_index;
	outfile2.close();

	return 0;
}