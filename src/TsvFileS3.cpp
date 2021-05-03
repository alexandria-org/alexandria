
#include "TsvFileS3.h"

TsvFileS3::TsvFileS3(const Aws::S3::S3Client &s3_client, const string &file_name) : TsvFile(file_name) {
	// Check if the file exists.

	m_s3_client = s3_client;

	ifstream infile(get_path());

	if (!infile.good()) {
		download_file();
	}

	set_file_name(get_path());
}

TsvFileS3::TsvFileS3(const Aws::S3::S3Client &s3_client, const string &bucket, const string &file_name)
: TsvFile(file_name) {
	// Check if the file exists.

	m_s3_client = s3_client;
	m_bucket = bucket;

	ifstream infile(get_path());

	if (!infile.good()) {
		download_file();
	}

	set_file_name(get_path());
}

TsvFileS3::~TsvFileS3() {
	
}

string TsvFileS3::get_path() const {
	return string(TSV_FILE_DESTINATION) + "/" + m_file_name;
}

int TsvFileS3::download_file() {
	Aws::S3::Model::GetObjectRequest request;

	const string bucket = get_bucket();

	cout << "Downloading file from " << bucket << " key: " << m_file_name << endl;
	request.SetBucket(bucket);
	request.SetKey(m_file_name);

	auto outcome = m_s3_client.GetObject(request);

	if (outcome.IsSuccess()) {

		ofstream outfile(get_path(), ios::trunc);

		if (outfile.good()) {
			auto &stream = outcome.GetResultWithOwnership().GetBody();

			outfile << stream.rdbuf();
		} else {
			return CC_ERROR;
		}

	} else {
		return CC_ERROR;
	}

	return CC_OK;
}

string TsvFileS3::get_bucket() {
	if (m_bucket.size()) return m_bucket;
	if (getenv("ALEXANDRIA_LIVE")) {
		return TSV_FILE_BUCKET;
	}
	return TSV_FILE_BUCKET_DEV;
}
