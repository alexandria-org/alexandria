
#include "CCApi.h"

CCApi::CCApi(const Aws::S3::S3Client &s3_client) {
	m_s3_client = s3_client;
}

CCApi::~CCApi() {

}

string CCApi::query(const string &query) {

	vector<string> words = get_words(query);

	for (const string &word : words) {
		
	}

	return "{\"results\": \""+query+"\"}";
}