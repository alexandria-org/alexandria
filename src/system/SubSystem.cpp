
#include "SubSystem.h"

SubSystem::SubSystem() {
	init_aws_api();

	setenv("AWS_EC2_METADATA_DISABLED", "true", 1);
	auto credentialsProvider = Aws::MakeShared<Aws::Auth::EnvironmentAWSCredentialsProvider>("asd");
	m_s3_client = new Aws::S3::S3Client(credentialsProvider, get_s3_config());

	TsvFileS3 domain_index(*m_s3_client, "domain_info.tsv");
	m_domain_index = new Dictionary(domain_index);

	TsvFileS3 dictionary(*m_s3_client, "dictionary.tsv");

	m_dictionary = new Dictionary(dictionary);

	dictionary.read_column_into(0, m_words);

	sort(m_words.begin(), m_words.end(), [](const string &a, const string &b) {
		return a < b;
	});
}

SubSystem::~SubSystem() {
	delete m_dictionary;
	delete m_domain_index;
	delete m_s3_client;

	deinit_aws_api();
}

const Dictionary *SubSystem::domain_index() const {
	return m_domain_index;
}

const Dictionary *SubSystem::dictionary() const {
	return m_dictionary;
}

const vector<string> SubSystem::words() const {
	return m_words;
}

const Aws::S3::S3Client SubSystem::s3_client() const {
	return *m_s3_client;
}


string SubSystem::download_to_string(const string &bucket, const string &key) const {

	Aws::S3::Model::GetObjectRequest request;
	cout << "Downloading " << bucket << " key: " << key << endl;
	request.SetBucket(bucket);
	request.SetKey(key);

	auto outcome = m_s3_client->GetObject(request);

	string ret;
	if (outcome.IsSuccess()) {

		auto &stream = outcome.GetResultWithOwnership().GetBody();

		filtering_istream decompress_stream;
		decompress_stream.push(gzip_decompressor());
		decompress_stream.push(stream);
		ret = string(istreambuf_iterator<char>(decompress_stream), {});
	}

	return ret;
}

bool SubSystem::download_to_stream(const string &bucket, const string &key, ofstream &output_stream) const {

	Aws::S3::Model::GetObjectRequest request;
	cout << "Downloading " << bucket << " key: " << key << endl;
	request.SetBucket(bucket);
	request.SetKey(key);

	auto outcome = m_s3_client->GetObject(request);

	string ret;
	if (outcome.IsSuccess()) {

		auto &stream = outcome.GetResultWithOwnership().GetBody();

		filtering_istream decompress_stream;
		decompress_stream.push(gzip_decompressor());
		decompress_stream.push(stream);

		output_stream << decompress_stream.rdbuf();

		return true;
	} else {
		cout << "NON SUCCESS" << endl;
	}

	return false;
}

void SubSystem::upload_from_string(const string &bucket, const string &key, const string &data) const {
	stringstream infile(data);

	filtering_istream in;
	in.push(gzip_compressor());
	in.push(infile);

	upload_from_stream(bucket, key, in);
}

void SubSystem::upload_from_stream(const string &bucket, const string &key, ifstream &file_stream) const {
	filtering_istream in;
	in.push(gzip_compressor());
	in.push(file_stream);

	upload_from_stream(bucket, key, in, 3);
}

void SubSystem::upload_from_stream(const string &bucket, const string &key, filtering_istream &compress_stream) const {
	upload_from_stream(bucket, key, compress_stream, 3);
}

void SubSystem::upload_from_stream(const string &bucket, const string &key, filtering_istream &compress_stream,
	size_t retries) const {

	cout << "Uploading " << bucket << " key: " << key << endl;

	Aws::S3::Model::PutObjectRequest request;
	request.SetBucket(bucket);
	request.SetKey(key);

	std::shared_ptr<Aws::StringStream> request_body = Aws::MakeShared<Aws::StringStream>("");
	*request_body << compress_stream.rdbuf();
	request.SetBody(request_body);

	Aws::S3::Model::PutObjectOutcome outcome = s3_client().PutObject(request);
	if (!outcome.IsSuccess()) {
		// Retry.
		if (retries > 0) {
			cout << "Upload failed, retrying for word: " << key << endl;
			compress_stream.seekg(0);
			upload_from_stream(bucket, key, compress_stream, retries - 1);
		}
	}
}

void SubSystem::init_aws_api() {
	Aws::SDKOptions options;
	options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Off;
	options.loggingOptions.logger_create_fn = get_logger_factory();

	Aws::InitAPI(options);
}

void SubSystem::deinit_aws_api() {
	Aws::SDKOptions options;
	Aws::ShutdownAPI(options);
}

Aws::Client::ClientConfiguration SubSystem::get_s3_config() {

	Aws::Client::ClientConfiguration config;
	config.region = "us-east-1";
	config.scheme = Aws::Http::Scheme::HTTP;
	config.verifySSL = false;
	config.requestTimeoutMs = 300000; // 5 minutes
	config.connectTimeoutMs = 300000; // 5 minutes

	return config;
}

