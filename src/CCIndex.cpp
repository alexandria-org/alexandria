
#include "CCIndex.h"

CCIndex::CCIndex() {
	m_stream.push(gzip_decompressor());
}

CCIndex::~CCIndex() {
}

void CCIndex::read_stream(basic_iostream< char, std::char_traits< char > > &stream) {
	m_stream.push(stream);
}

