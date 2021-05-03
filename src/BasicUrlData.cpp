
#include "BasicUrlData.h"


BasicUrlData::BasicUrlData(const SubSystem *sub_system) :
	BasicData(sub_system)
{
}

BasicUrlData::~BasicUrlData() {
}

/*
Diskutera med Ivan:

1. Vi tar tre första orden och sen kollar om något av dem är i dictionary va?
2. Snippet, enklaste formen?
3. Ska vi plocka både 
*/

string BasicUrlData::build_index(int shard, int id) {

	map<string, int> word_counts;

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

	ofstream outfile;
	string file_name = get_output_filename(shard, id);
	outfile.open(file_name, ios::trunc);
	if (outfile.is_open()) {
		outfile << m_result.str();
	} else {
		cout << "Error: " << strerror(errno) << endl;
		cout << "outfile is not open" << endl;
	}
	outfile.close();

	return file_name;
}

inline string BasicUrlData::make_snippet(const string &text_after_h1) {
	return text_after_h1.substr(0, 200);
}

void BasicUrlData::add_to_index(const string &word, const URL &url, const string &title, const string &snippet) {

	const auto iter = m_sub_system->domain_index()->find(url.host_reverse());
	if (iter == m_sub_system->domain_index()->end()) return;

	const DictionaryRow row = iter->second;

	const int harmonic = row.get_int(1);

	if (harmonic <= CC_HARMONIC_LIMIT) {
		return;
	}

	m_result << word << "\t" << url << "\t" << harmonic << "\t" << title << "\t" << snippet << endl;
}

inline bool BasicUrlData::is_in_dictionary(const string &word) {
	return m_sub_system->dictionary()->has_key(word);
}

string BasicUrlData::get_output_filename(int shard, int id) {
	return "/mnt/"+to_string(shard)+"/output_"+to_string(id)+".tsv";
}

