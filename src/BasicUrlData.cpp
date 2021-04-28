
#include "BasicUrlData.h"


BasicUrlData::BasicUrlData() {

}

BasicUrlData::~BasicUrlData() {
}

void BasicUrlData::build_index() {

	map<string, int> word_counts;

	for (const string &line : m_data) {

		stringstream ss(line);

		string col;
		getline(ss, col, '\t');

		URL url(col);

		string title;
		getline(ss, title, '\t');

		vector<string> title_words = get_words(title, 3);

		for (const string &word : title_words) {
			word_counts[word]++;
		}
		//cout << url.host() << " title: " << title << endl;
	}

	typedef pair<string, int> pair_t;
	vector<pair_t> vec;

	copy(word_counts.begin(), word_counts.end(), std::back_inserter(vec));

	std::sort(vec.begin(), vec.end(), [](const pair_t& l, const pair_t& r) {
		if (l.second != r.second) {
			return l.second < r.second;
		}

		return l.first < r.first;
	});


	for (const auto &iter : vec) {
		cout << iter.first << "\t" << iter.second << endl;
	}

	cout << "total number of words: " << word_counts.size() << endl;

}
