
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

void CCIndex::read_data(filtering_istream &decompress_stream) {

	for (string line; getline(decompress_stream, line); ) {
		istringstream ss(line);
		vector<string> row;
		for (string col; getline(ss, col, '\t'); ) {
			row.push_back(col);
		}
		m_data.push_back(row);
	}

}
