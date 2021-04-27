
#include "CCIndex.h"


CCIndex::CCIndex() {
	m_group_by = CC_COLUMN_URL;
	m_columns = {
		CC_COLUMN_URL,
		CC_COLUMN_TITLE,
		CC_COLUMN_H1,
		CC_COLUMN_META,
		CC_COLUMN_TEXT
	};
}

CCIndex::~CCIndex() {
}

void CCIndex::read_stream(basic_iostream< char, std::char_traits< char > > &stream) {
	filtering_istream decompress_stream;
	decompress_stream.push(gzip_decompressor());
	decompress_stream.push(stream);

	read_data(decompress_stream);
}

void CCIndex::read_stream(ifstream &stream) {
	filtering_istream decompress_stream;
	decompress_stream.push(gzip_decompressor());
	decompress_stream.push(stream);

	read_data(decompress_stream);
}

void CCIndex::build_index() {
	
	int i = 0;
	for (const vector<string> &cols : m_data) {
		string group_by;
		for (int i = 0; i < m_columns.size(); i++) {
			int col_type = m_columns[i];
			if (col_type == m_group_by) {
				group_by = cols[i];
			} else {
				vector<string> words = get_words(cols[i]);
				for (const string &word : words) {
					index_word(word, group_by, m_columns[i]);
				}
			}
		}
	}

	//sort(m_index.begin(), m_index.end(), compare_numeric);
}

void CCIndex::read_data(filtering_istream &decompress_stream) {

	int i = 0;
	for (string line; getline(decompress_stream, line); ) {
		istringstream ss(line);
		vector<string> row;
		for (string col; getline(ss, col, '\t'); ) {
			row.push_back(col);
		}
		row.resize(5, "");
		m_data.push_back(row);
	}

}

void CCIndex::index_word(const string &word, const string &group_by, int col_type) {
	m_index[word]++;
}

bool CCIndex::compare_numeric(pair<string, int>& a, pair<string, int>& b) {
	return a.second > b.second;
}
