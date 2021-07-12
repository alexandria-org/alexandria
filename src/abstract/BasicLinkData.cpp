
#include "BasicLinkData.h"


BasicLinkData::BasicLinkData() {

}

BasicLinkData::BasicLinkData(const SubSystem *sub_system) :
	BasicData(sub_system)
{
}

BasicLinkData::~BasicLinkData() {
}

void BasicLinkData::build_index(const string &output_file_name) {

	map<string, int> word_counts;

	int added_words = 0;
	int not_added_words = 0;
	for (const string &line : m_data) {

		stringstream ss(line);

		string from_domain;
		getline(ss, from_domain, '\t');

		string from_uri;
		getline(ss, from_uri, '\t');

		string to_domain;
		getline(ss, to_domain, '\t');

		string to_uri;
		getline(ss, to_uri, '\t');

		string link_text;
		getline(ss, link_text, '\t');

		vector<string> link_words = get_words_without_stopwords(link_text, 3);

		for (const string &word : link_words) {
			if (is_in_dictionary(word)) {
				add_to_index(word, from_domain, from_uri, to_domain, to_uri, link_text);
				added_words++;
			} else {
				not_added_words++;
			}
			word_counts[word]++;
		}
	}

	ofstream outfile;
	outfile.open(output_file_name, ios::trunc);
	if (outfile.is_open()) {
		outfile << m_result.str();
	} else {
		cout << "Error: " << strerror(errno) << endl;
		cout << "outfile is not open" << endl;
	}
}

void BasicLinkData::add_to_index(const string &word, const string from_domain, const string &from_uri,
	const string &to_domain, const string &to_uri, const string &link_text) {

	const auto iter = m_sub_system->domain_index()->find(URL::host_reverse(from_domain));

	float harmonic;
	if (iter == m_sub_system->domain_index()->end()) {
		#ifndef CC_TESTING
			return;
			//harmonic = 0;
		#else
			harmonic = 0.0f;
		#endif
	} else {
		const DictionaryRow row = iter->second;
		harmonic = row.get_float(1);
	}

	m_result << word << "\t" << to_domain << "\t" << to_uri << "\t" << from_domain << "\t" << from_uri << "\t"
		<< harmonic << "\t" << link_text << endl;
}

inline bool BasicLinkData::is_in_dictionary(const string &word) {
	return m_sub_system->dictionary()->has_key(word);
}
