
#include "CCIndex.h"


CCIndex::CCIndex() {
	
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

		const string url = cols[0];
		const string title = cols[1];
		const string h1 = cols[2];
		const string meta = cols[3];
		const string text = cols[4];

		vector<string> words = get_words(text);
	}
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
