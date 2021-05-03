
#include "CCIndexer.h"

CCIndexer::CCIndexer(const SubSystem *sub_system, const string &bucket, const string &key, int shard, int id) :
m_sub_system(sub_system), m_url_data(sub_system)
{
	m_id = id;
	m_shard = shard;
	m_bucket = bucket;

	m_key = key;
	m_key.replace(key.find(".warc.gz"), 8, ".gz");

	m_link_key = key;
	m_link_key.replace(key.find(".warc.gz"), 8, ".links.gz");

}

CCIndexer::~CCIndexer() {
}

string CCIndexer::run() {

	download_file(m_key, m_url_data);
	//download_file(m_key, m_link_data);

	//m_link_data.build_index();


	return m_url_data.build_index(m_shard, m_id);
}

void CCIndexer::download_file(const string &key, BasicData &data) {

	Aws::S3::Model::GetObjectRequest request;
	cout << "Downloading " << m_bucket << " key: " << key << endl;
	request.SetBucket(m_bucket);
	request.SetKey(key);

	auto outcome = m_sub_system->s3_client().GetObject(request);

	if (outcome.IsSuccess()) {

		auto &stream = outcome.GetResultWithOwnership().GetBody();
		data.read_stream(stream);

	}

}
