
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

	for (const auto &iter : vec) {
		result += iter.first + "\t" + to_string(iter.second) + "\n";
	}

	ofstream outfile;
	outfile.open("/host/alexandria.org/main_dictionary.tsv", ios::app);
	outfile << result;
	outfile.close();

	return 0;
}