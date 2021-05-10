
#include "CCLinkIndexRunner.h"

CCLinkIndexRunner::CCLinkIndexRunner() {
}

CCLinkIndexRunner::~CCLinkIndexRunner() {
}

void CCLinkIndexRunner::run_all() {
	run_all(0);
}

void CCLinkIndexRunner::run_all(size_t limit) {

	init_aws_api();

	Aws::S3::S3Client s3_client = get_s3_client();

	m_sub_system = new SubSystem(s3_client);

	//map<int, vector<string>> downloaded_files = download_all(limit);
	//index_all(downloaded_files);
	//sort_all();
	upload_all();

	delete m_sub_system;

	deinit_aws_api();

}

map<int, vector<string>> CCLinkIndexRunner::download_all(size_t limit) {
	string warc_paths_url = "crawl-data/CC-MAIN-2021-10/warc.paths.gz";
	TsvFileS3 warc_paths_file(m_sub_system->s3_client(), "commoncrawl", warc_paths_url);

	vector<string> warc_paths;
	warc_paths_file.read_column_into(0, warc_paths);

	ThreadPool pool(CC_NUM_THREADS_DOWNLOADING);
	std::vector<std::future<string>> results;

	int id = 1;
	const int num_shards = 8;
	map<int, vector<string>> shard_files;
	for (const string &warc_path : warc_paths) {

		int shard = id % num_shards;

		results.emplace_back(
			pool.enqueue([this, warc_path, shard, id] {
				return run_download_thread(warc_path, shard, id);
			})
		);
		
		string output_file = "/mnt/"+to_string(shard)+"/input/links_"+to_string(id)+".tsv";
		shard_files[shard].push_back(output_file);

		if (limit && id >= limit) break;
		id++;
	}

	for(auto && result: results) {
		cout << "Finished downloading: " << result.get() << endl;
	}

	return shard_files;
}

void CCLinkIndexRunner::index_all(const map<int, vector<string>> &files) {
	Profiler split_profiler("Split files");

	vector<thread> threads;
	for (const auto &iter : files) {
		thread th (&CCLinkIndexRunner::run_indexer_thread, this, iter.second, iter.first);
		threads.push_back(move(th));
	}

	// join threads.
	int joined = 0;
	for (thread &_th : threads) {
		cout << "threads joined: " << joined << endl;
		joined++;
		_th.join();
	}
	threads.clear();

	split_profiler.stop();
}

void CCLinkIndexRunner::sort_all() {
	Profiler profiler("Sorting");
	vector<thread> threads;
	vector<string> words = m_sub_system->words();
	vector<vector<string>> chunks;

	vector_chunk(words, ceil((float)words.size() / CC_NUM_THREADS_SORTING), chunks);

	cout << "chunks: " << chunks.size() << endl;
	cout << "Sorting" << endl;

	int id = 0;
	for (const vector<string> &chunk : chunks) {
		cout << "Running chunk: " << id << endl;
		thread th (&CCLinkIndexRunner::run_sorter_thread, this, chunk);
		threads.push_back(move(th));
		id++;
	}

	// join threads.
	int joined = 0;
	for (thread &_th : threads) {
		cout << "threads joined: " << joined << endl;
		joined++;
		_th.join();
	}
	threads.clear();
}

void CCLinkIndexRunner::upload_all() {
	// Uploading

	Profiler profile("Uploading");

	ThreadPool pool(CC_NUM_THREADS_UPLOADING);
	std::vector<std::future<string>> results;

	int idx = 0;
	size_t word_size = m_sub_system->words().size();
	for (const string &word : m_sub_system->words()) {

		results.emplace_back(
			pool.enqueue([this, word, idx, word_size] {
				upload_results_thread(word, 3);
				return word + " done " + to_string(idx) + " out of " + to_string(word_size);
			})
		);
		idx++;
	}

	for (auto && result: results) {
		cout << result.get() << endl;
	}
}

string CCLinkIndexRunner::run_download_thread(const string &warc_path, int shard, int id) {
	const string bucket = "alexandria-cc-output";
	CCLinkIndexer indexer(m_sub_system);
	return indexer.download(bucket, warc_path, shard, id);
}

void CCLinkIndexRunner::run_indexer_thread(const vector<string> &file_names, int shard) {

	CCLinkIndexer indexer(m_sub_system);
	indexer.index(file_names, shard);
}

void CCLinkIndexRunner::run_sorter_thread(const vector<string> &chunk) {

	try {
		CCLinkIndexer indexer(m_sub_system);
		indexer.sorter(chunk);
	} catch (runtime_error &error) {
		cout << error.what() << endl;
	}
}

void CCLinkIndexRunner::upload_results_thread(const string &word, int retries) {

	Aws::S3::Model::PutObjectRequest request;
	request.SetBucket("alexandria-index");
	string key = "CC-MAIN-2021-10/index_" + word + ".link.tsv.gz";
	request.SetKey(key);

	ifstream infile;
	for (int shard = 0; shard < 8; shard++) {
		infile.open("/mnt/"+to_string(shard)+"/upload/links_" + word + ".tsv");
		if (infile.is_open()) {
			break;
		}
	}

	if (!infile.is_open()) {
		cout << "ERROR could not find output file for word: " << word << endl;
		return;
	}

	filtering_istream in;
	in.push(gzip_compressor());
	in.push(infile);

	std::shared_ptr<Aws::StringStream> request_body = Aws::MakeShared<Aws::StringStream>("");
	*request_body << in.rdbuf();
	request.SetBody(request_body);

	Aws::S3::Model::PutObjectOutcome outcome = m_sub_system->s3_client().PutObject(request);
	if (!outcome.IsSuccess()) {
		// Retry.
		if (retries > 0) {
			infile.close();
			cout << "Upload failed, retrying for word: " << word << endl;
			upload_results_thread(word, retries - 1);
		}
	}
}
