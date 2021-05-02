
#include "BasicUrlData.h"


BasicUrlData::BasicUrlData() :
	m_domain_file("/mnt/00/database/domain_info.tsv")
{
	load_dictionary();
}

BasicUrlData::~BasicUrlData() {
}

/*
Diskutera med Ivan:

1. Vi tar tre första orden och sen kollar om något av dem är i dictionary va?
2. Snippet, enklaste formen?
3. Ska vi plocka både 
*/

void BasicUrlData::build_index(int id) {

	map<string, int> word_counts;

	load_domain_meta();

	int added_words = 0;
	int not_added_words = 0;
	for (const string &line : m_data) {

		stringstream ss(line);

		string col;
		getline(ss, col, '\t');

		URL url(col);



		string title;
		getline(ss, title, '\t');
		string h1;
		getline(ss, h1, '\t');
		string meta;
		getline(ss, meta, '\t');
		string text_after_h1;
		getline(ss, text_after_h1, '\t');

		vector<string> title_words = get_words(title, 3);

		for (const string &word : title_words) {
			if (is_in_dictionary(word)) {
				add_to_index(word, url, title.substr(0, 60), make_snippet(text_after_h1));
				added_words++;
			} else {
				not_added_words++;
			}
			word_counts[word]++;
		}
	}

	cout << "Added words: " << added_words << endl;
	cout << "Threw away words: " << not_added_words << endl;

	/*

	typedef pair<string, int> pair_t;
	vector<pair_t> vec;

	copy(word_counts.begin(), word_counts.end(), std::back_inserter(vec));

	std::sort(vec.begin(), vec.end(), [](const pair_t& l, const pair_t& r) {
		if (l.second != r.second) {
			return l.second < r.second;
		}

		return l.first < r.first;
	});

	string result;

	for (const auto &iter : vec) {
		result += iter.first + "\t" + to_string(iter.second) + "\n";
	}

	ofstream outfile;
	string file_name = "/mnt/alexandria_main/output_"+to_string(id)+".tsv";
	outfile.open(file_name, ios::trunc);
	if (outfile.is_open()) {
		outfile << result;
	} else {
		cout << "Error: " << strerror(errno) << endl;
		cout << "outfile is not open" << endl;
	}
	outfile.close();

	*/

	ofstream outfile;
	string file_name = "/mnt/00/output/output_"+to_string(id)+".tsv";
	outfile.open(file_name, ios::trunc);
	if (outfile.is_open()) {
		outfile << m_result.str();
	} else {
		cout << "Error: " << strerror(errno) << endl;
		cout << "outfile is not open" << endl;
	}
	outfile.close();

}

inline string BasicUrlData::make_snippet(const string &text_after_h1) {
	return text_after_h1.substr(0, 200);
}

void BasicUrlData::load_domain_meta() {
	set<string> hosts;
	for (const string &line : m_data) {

		stringstream ss(line);

		string col;
		getline(ss, col, '\t');

		URL url(col);

		string host = url.host_reverse();
		hosts.insert(host);
	}

	map<string, string> rows = m_domain_file.find_all(hosts);

	for (const auto &iter : rows) {
		stringstream ss(iter.second);
		string domain;
		double pagerank;
		int harmonic;

		ss >> domain >> pagerank >> harmonic;

		m_domain_meta[iter.first] = harmonic;
	}
}

void BasicUrlData::add_to_index(const string &word, const URL &url, const string &title, const string &snippet) {

	const int harmonic = m_domain_meta[url.host_reverse()];

	if (harmonic <= CC_HARMONIC_LIMIT) {
		return;
	}

	m_result << word << "\t" << url << "\t" << harmonic << "\t" << title << "\t" << snippet << endl;
}

inline bool BasicUrlData::is_in_dictionary(const string &word) {
	return m_dictionary.find(word) != m_dictionary.end();
}

void BasicUrlData::load_dictionary() {
	TsvFile dictionary("/mnt/00/database/dictionary.tsv");
	dictionary.read_column_into(0, m_dictionary);
}
