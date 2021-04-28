
#include "BasicData.h"

void BasicData::read_stream(basic_iostream< char, std::char_traits< char > > &stream) {
	filtering_istream decompress_stream;
	decompress_stream.push(gzip_decompressor());
	decompress_stream.push(stream);

	read_data(decompress_stream);
}

void BasicData::read_stream(ifstream &stream) {
	filtering_istream decompress_stream;
	decompress_stream.push(gzip_decompressor());
	decompress_stream.push(stream);

	read_data(decompress_stream);
}

void BasicData::read_data(filtering_istream &decompress_stream) {

	int i = 0;
	for (string line; getline(decompress_stream, line); ) {
		m_data.push_back(line);
	}

}
