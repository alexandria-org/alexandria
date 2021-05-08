
#include "CCLinkIndexer.h"

void run_download_thread_link(const SubSystem *sub_system, const string &warc_path, int shard, int id) {
	const string bucket = "alexandria-cc-output";
	CCLinkIndexer indexer(sub_system);
	indexer.download(bucket, warc_path, shard, id);
}

void run_indexer_thread_link(const SubSystem *sub_system, const vector<string> &chunk, const vector<string> &input_files,
		int shard) {

	CCLinkIndexer indexer(sub_system);
	indexer.index(chunk, input_files, shard);
}

void run_sorter_thread_link(const SubSystem *sub_system, const vector<string> &chunk, int shard) {

	CCLinkIndexer indexer(sub_system);
	indexer.sorter(chunk, shard);

}

void upload_results_thread_link(SubSystem *sub_system, const string &word, int retries) {

	Aws::S3::Model::PutObjectRequest request;
	request.SetBucket("alexandria-index");
	string key = "CC-MAIN-2021-10/index_" + word + ".link.tsv.gz";
	request.SetKey(key);

	ifstream infile;
	for (int shard = 0; shard < 8; shard++) {
		infile.open("/mnt/" + to_string(shard) + "/output/links_" + word + ".tsv");
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

	Aws::S3::Model::PutObjectOutcome outcome = sub_system->s3_client().PutObject(request);
	if (!outcome.IsSuccess()) {
		// Retry.
		if (retries > 0) {
			cout << "Upload failed, retrying for word: " << word << endl;
			upload_results_thread_link(sub_system, word, retries - 1);
		}
	}
}

void CCLinkIndexer::run_all() {
	auto total_timer_start = std::chrono::high_resolution_clock::now();

	vector<thread> threads;

	Aws::SDKOptions options;
	options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Error;
	options.loggingOptions.logger_create_fn = get_logger_factory();

	Aws::InitAPI(options);

	Aws::S3::S3Client s3_client = get_s3_client();

	SubSystem *sub_system = new SubSystem(s3_client);

	string warc_paths_url = "crawl-data/CC-MAIN-2021-10/warc.paths.gz";
	TsvFileS3 warc_paths_file(s3_client, "commoncrawl", warc_paths_url);

	vector<string> warc_paths;
	warc_paths_file.read_column_into(0, warc_paths);

	int id = 1;
	const int num_shards = 8;
	vector<string> input_files;
	for (const string &warc_path : warc_paths) {

		int shard = id % num_shards;

		thread th(run_download_thread_link, sub_system, warc_path, shard, id);

		threads.push_back(move(th));

		if (threads.size() == CC_NUM_THREADS_DOWNLOADING) {
			// join threads.
			for (thread &_th : threads) {
				_th.join();
			}
			threads.clear();
		}
		
		string output_file = "/mnt/"+to_string(shard)+"/links_"+to_string(id)+".tsv";
		input_files.push_back(output_file);
		cout << output_file << endl;
		//if (id >= 1000) break;
		id++;
	}

	// join threads.
	for (thread &_th : threads) {
		_th.join();
	}
	threads.clear();

	cout << "Splitting dictionary into chunks" << endl;

	vector<string> words = sub_system->words();
	vector<vector<string>> chunks;

	cout << "words: " << words.size() << endl;

	vector_chunk(words, ceil((float)words.size() / CC_NUM_THREADS_INDEXING), chunks);

	cout << "chunks: " << chunks.size() << endl;

	id = 0;
	for (const vector<string> &chunk : chunks) {
		cout << "Running chunk: " << id << endl;
		int shard = id % num_shards;
		thread th (run_indexer_thread_link, sub_system, chunk, input_files, shard);
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

	cout << "Sorting" << endl;

	id = 0;
	for (const vector<string> &chunk : chunks) {
		cout << "Running chunk: " << id << endl;
		int shard = id % num_shards;
		thread th (run_sorter_thread_link, sub_system, chunk, shard);
		threads.push_back(move(th));
		id++;
	}

	// join threads.
	joined = 0;
	for (thread &_th : threads) {
		cout << "threads joined: " << joined << endl;
		joined++;
		_th.join();
	}
	threads.clear();

	// Uploading
	auto timer_start = std::chrono::high_resolution_clock::now();

	ThreadPool pool(CC_NUM_THREADS_UPLOADING);
	std::vector<std::future<string>> results;

	int idx = 0;
	size_t word_size = sub_system->words().size();
	for (const string &word : sub_system->words()) {

		results.emplace_back(
			pool.enqueue([sub_system, word, idx, word_size] {
				upload_results_thread_link(sub_system, word, 3);
				return word + " done " + to_string(idx) + " out of " + to_string(word_size);
			})
		);
		idx++;
	}

	for(auto && result: results) {
		cout << result.get() << endl;
	}

	auto timer_elapsed = std::chrono::high_resolution_clock::now() - timer_start;
	auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(timer_elapsed).count();

	cout << "upload took: " << microseconds / 1000 << " milliseconds" << endl;

	delete sub_system;

	Aws::ShutdownAPI(options);

	auto total_timer_elapsed = chrono::high_resolution_clock::now() - total_timer_start;
	microseconds = std::chrono::duration_cast<std::chrono::microseconds>(total_timer_elapsed).count();

	cout << "everything took: " << microseconds / 1000 << " milliseconds" << endl;
}

CCLinkIndexer::CCLinkIndexer(const SubSystem *sub_system) :
m_sub_system(sub_system), m_link_data(sub_system)
{
}

CCLinkIndexer::~CCLinkIndexer() {
}

void CCLinkIndexer::download(const string &bucket, const string &file, int shard, int id) {

	string link_key = file;
	link_key.replace(link_key.find(".warc.gz"), 8, ".links.gz");

	download_file(bucket, link_key, m_link_data);
	//download_file(m_key, m_link_data);

	//m_link_data.build_index();

	m_link_data.build_index(shard, id);
}

void CCLinkIndexer::index(const vector<string> &words, const vector<string> &input_files, int shard) {

	// Open all my output files.
	map<string, ofstream> out_files;
	for (const string &word : words) {
		string file_name = "/mnt/"+to_string(shard)+"/output/links_" + word + ".tsv";

		out_files[word].open(file_name, ios::trunc);

		if (!out_files[word].is_open()) {
			throw runtime_error("Could not open file: " + file_name + " error: " + strerror(errno));
		}
	}

	map<string, string> cache;

	const size_t max_cache_size = 10000000;
	size_t cache_size = 0;

	for (const string &input_file_name : input_files) {
		ifstream input_file(input_file_name);

		if (!input_file.is_open()) {
			throw runtime_error("Could not open file: " + input_file_name + " error: " + strerror(errno));
		}

		string line;
		while (getline(input_file, line)) {
			stringstream ss(line);
			string word;
			getline(ss, word, '\t');
			if (out_files.find(word) != out_files.end()) {
				cache[word] += line + '\n';
				cache_size++;
				//out_files[word] << line << endl;
			}
		}

		if (cache_size > max_cache_size) {
			// Flush cache.
			for (auto &iter : out_files) {
				iter.second << cache[iter.first];
			}
			cache.clear();
			cache_size = 0;
		}

		input_file.close();
	}

	// Flush cache.
	for (auto &iter : out_files) {
		iter.second << cache[iter.first];
	}
	cache.clear();
	cache_size = 0;

	for (auto &iter : out_files) {
		iter.second.close();
	}

}

void CCLinkIndexer::sorter(const vector<string> &words, int shard) {

	// Open all my output files.
	map<string, ofstream> out_files;
	for (const string &word : words) {
		string file_name = "/mnt/"+to_string(shard)+"/output/links_" + word + ".tsv";
		ifstream input_file(file_name);

		if (!input_file.is_open()) {
			throw runtime_error("Could not open file: " + file_name + " error: " + strerror(errno));
		}

		string line;
		vector<string> lines;
		vector<size_t> indices;
		vector<double> scores;
		size_t index = 0;
		while (getline(input_file, line)) {
			stringstream ss(line);
			string col;
			getline(ss, col, '\t'); // word
			getline(ss, col, '\t'); // from_domain
			getline(ss, col, '\t'); // from_uri
			getline(ss, col, '\t'); // score
			double score = stod(col);
			scores.push_back(score);
			indices.push_back(index);
			lines.push_back(line);
			index++;
		}
		input_file.close();

		sort(indices.begin(), indices.end(), [&](const size_t& a, const size_t& b) {
			return (scores[a] > scores[b]);
		});

		ofstream output_file(file_name, ios::trunc);
		const size_t max_num_lines = 500000;
		size_t line_num = 0;
		for (size_t i : indices) {
			output_file << lines[i] << endl;
			line_num++;
			if (line_num >= max_num_lines) break;
		}
	}

}

void CCLinkIndexer::download_file(const string &bucket, const string &key, BasicData &data) {

	Aws::S3::Model::GetObjectRequest request;
	cout << "Downloading " << bucket << " key: " << key << endl;
	request.SetBucket(bucket);
	request.SetKey(key);

	auto outcome = m_sub_system->s3_client().GetObject(request);

	if (outcome.IsSuccess()) {

		auto &stream = outcome.GetResultWithOwnership().GetBody();
		data.read_stream(stream);

	}

}
