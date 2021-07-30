
#include "BasicUrlData.h"
#include "text/Text.h"


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

void BasicUrlData::build_index(const string &output_file_name) {

	m_next_key = 0;
	m_keys.clear();
	m_index.clear();

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

		vector<string> title_words = Text::get_words_without_stopwords(title, 3);

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
	outfile.open(output_file_name, ios::trunc);
	if (outfile.is_open()) {
		outfile << m_result.str();
	} else {
		cout << "Error: " << strerror(errno) << endl;
		cout << "outfile is not open" << endl;
	}
}

string BasicUrlData::build_full_text_index() {

	m_next_key = 0;
	m_keys.clear();
	m_index.clear();

	int added_words = 0;
	int not_added_words = 0;
	hash<string> hasher;
	for (const string &line : m_data) {

		stringstream ss(line);

		string col;
		getline(ss, col, '\t');

		URL url(col);

		uint64_t id = hasher(col);
		float score = 0.0f;

		const auto iter = m_sub_system->domain_index()->find(url.host_reverse());

		float harmonic;
		if (iter == m_sub_system->domain_index()->end()) {
			#ifndef CC_TESTING
				continue;
			#else
				score = 0;
			#endif
		} else {
			const DictionaryRow row = iter->second;
			score = (float)row.get_float(1);
		}

		string title;
		getline(ss, title, '\t');
		string h1;
		getline(ss, h1, '\t');
		string meta;
		getline(ss, meta, '\t');
		string text_after_h1;
		getline(ss, text_after_h1, '\t');

		vector<string> title_words = Text::get_words_without_stopwords(title);
		vector<string> h1_words = Text::get_words_without_stopwords(h1);
		vector<string> meta_words = Text::get_words_without_stopwords(meta);
		vector<string> text_after_h1_words = Text::get_words_without_stopwords(text_after_h1);

		for (const string &word : title_words) {
			add_to_full_text_index(word, id, score);
		}
		for (const string &word : h1_words) {
			add_to_full_text_index(word, id, score);
		}
		for (const string &word : meta_words) {
			add_to_full_text_index(word, id, score);
		}
		for (const string &word : text_after_h1_words) {
			add_to_full_text_index(word, id, score);
		}
	}

	string file_name = store_full_text();

	return file_name;
}

inline string BasicUrlData::make_snippet(const string &text_after_h1) {
	return text_after_h1.substr(0, 200);
}

void BasicUrlData::add_to_index(const string &word, const URL &url, const string &title, const string &snippet) {

	const auto iter = m_sub_system->domain_index()->find(url.host_reverse());

	float harmonic;
	if (iter == m_sub_system->domain_index()->end()) {
		#ifndef CC_TESTING
			return;
		#else
			harmonic = 0.0f;
		#endif
	} else {
		const DictionaryRow row = iter->second;
		harmonic = row.get_float(1);
	}

	if (harmonic < CC_HARMONIC_LIMIT) {
		return;
	}

	m_result << word << "\t" << url << "\t" << harmonic << "\t" << title << "\t" << snippet << endl;
}

inline void BasicUrlData::add_to_full_text_index(const string &word, uint64_t id, float score) {
	m_full_text_result << word << "\t" << id << "\t" << score << endl;
}

inline bool BasicUrlData::is_in_dictionary(const string &word) {
	return m_sub_system->dictionary()->has_key(word);
}

string BasicUrlData::sort_and_store() {
	/*
	vector<size_t> indices;
	for (size_t index = 0; index < m_keys.size(); index++) {
		indices.push_back(index);
	}

	sort(indices.begin(), indices.end(), [&](const size_t& a, const size_t& b) {
		return (m_keys[a] < m_keys[b]);
	});

	ofstream outfile;
	string file_name = get_output_filename();
	outfile.open(file_name, ios::trunc);
	if (outfile.is_open()) {
		for (const size_t index : indices) {
			outfile << m_index[index];
		}
		//outfile << m_result.str();
	} else {
		cout << "Error: " << strerror(errno) << endl;
		cout << "outfile is not open" << endl;
	}
	outfile.close();

	return file_name;*/
	return "";
}

string BasicUrlData::store_full_text() {

	/*ofstream outfile;
	string file_name = get_output_filename();
	outfile.open(file_name, ios::trunc);
	if (outfile.is_open()) {
		outfile << m_full_text_result.str();
	} else {
		cout << "Error: " << strerror(errno) << endl;
		cout << "outfile is not open" << endl;
	}
	outfile.close();

	return file_name;
	*/
	return "";
}

