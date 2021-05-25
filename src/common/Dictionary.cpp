
#include "Dictionary.h"

Dictionary::Dictionary() {

}

Dictionary::Dictionary(TsvFile &tsv_file) {
	while (!tsv_file.eof()) {
		string line = tsv_file.get_line();
		stringstream ss(line);
		string col;
		getline(ss, col, '\t');

		if (col.size()) {
			size_t key = hash<string>{}(col);

			if (m_rows.find(key) != m_rows.end()) {
				handle_collision(key, col);
			}

			m_rows[key] = DictionaryRow(ss);
		}
	}
}

Dictionary::~Dictionary() {

}


unordered_map<size_t, DictionaryRow>::const_iterator Dictionary::find(const string &key) const {
	return m_rows.find(hash<string>{}(key));
}

unordered_map<size_t, DictionaryRow>::const_iterator Dictionary::begin() const {
	return m_rows.begin();
}

unordered_map<size_t, DictionaryRow>::const_iterator Dictionary::end() const {
	return m_rows.end();
}

bool Dictionary::has_key(const string &key) const {
	return find(key) != end();
}

void Dictionary::handle_collision(size_t key, const string &col) {
	cout << "Collision: " << key << " " << col << endl;
}
