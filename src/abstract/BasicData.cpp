
#include "BasicData.h"

BasicData::BasicData() {

}

BasicData::BasicData(const SubSystem *sub_system) :
	m_sub_system(sub_system)
{
}

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

void BasicData::download(const string &bucket, const string &key) {

	Aws::S3::Model::GetObjectRequest request;
	cout << "Downloading " << bucket << " key: " << key << endl;
	request.SetBucket(bucket);
	request.SetKey(key);

	auto outcome = m_sub_system->s3_client().GetObject(request);

	if (outcome.IsSuccess()) {

		auto &stream = outcome.GetResultWithOwnership().GetBody();
		read_stream(stream);

	}

}


void BasicData::read_data(filtering_istream &decompress_stream) {

	int i = 0;
	for (string line; getline(decompress_stream, line); ) {
		m_data.push_back(line);
	}

}
