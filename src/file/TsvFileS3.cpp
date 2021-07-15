
#include "TsvFileS3.h"

TsvFileS3::TsvFileS3(const Aws::S3::S3Client &s3_client, const string &file_name) {
	// Check if the file exists.

	m_s3_client = s3_client;
	m_file_name = file_name;

	ifstream infile(get_path());

	if (!infile.good()) {
		download_file();
	}

	set_file_name(get_path());
}

TsvFileS3::TsvFileS3(const Aws::S3::S3Client &s3_client, const string &bucket, const string &file_name) {
	// Check if the file exists.

	m_s3_client = s3_client;
	m_bucket = bucket;
	m_file_name = file_name;

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

	if (m_file_name.find(".gz") == m_file_name.size() - 3) {
		m_is_gzipped = true;
	} else {
		m_is_gzipped = false;
	}

	cout << "Downloading file from " << bucket << " key: " << m_file_name << endl;
	request.SetBucket(bucket);
	request.SetKey(m_file_name);

	auto outcome = m_s3_client.GetObject(request);

	if (outcome.IsSuccess()) {

		create_directory();
		ofstream outfile(get_path(), ios::trunc);

		if (outfile.good()) {
			auto &stream = outcome.GetResultWithOwnership().GetBody();

			if (m_is_gzipped) {
				filtering_istream in;
				in.push(gzip_decompressor());
				in.push(stream);
				outfile << in.rdbuf();
			} else {
				outfile << stream.rdbuf();
			}
		} else {
			return CC_ERROR;
		}

	} else {
		return CC_ERROR;
	}

	cout << "Done downloading file from " << bucket << " key: " << m_file_name << endl;

	return CC_OK;
}

string TsvFileS3::get_bucket() {
	if (m_bucket.size()) return m_bucket;
	if (getenv("ALEXANDRIA_LIVE") != NULL && stoi(getenv("ALEXANDRIA_LIVE")) > 0) {
		return TSV_FILE_BUCKET;
	}
	return TSV_FILE_BUCKET_DEV;
}

void TsvFileS3::create_directory() {
	boost::filesystem::path path(get_path());
	boost::filesystem::create_directories(path.parent_path());
}
