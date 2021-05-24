
#include "CCIndexRunner.h"

template <class TemplateIndexer>
CCIndexRunner<TemplateIndexer>::CCIndexRunner(const string &cc_batch)
: m_cc_batch(cc_batch)
{
}

template<class TemplateIndexer>
CCIndexRunner<TemplateIndexer>::~CCIndexRunner() {
}

template<class TemplateIndexer>
void CCIndexRunner<TemplateIndexer>::run_all() {
	run_all(0);
}

template<class TemplateIndexer>
void CCIndexRunner<TemplateIndexer>::run_all(size_t limit) {

	init_aws_api();

	Aws::S3::S3Client s3_client = get_s3_client();

	m_sub_system = new SubSystem(s3_client);

	map<int, vector<string>> downloaded_files = download_all(limit);
	/*for (const auto &iter : downloaded_files) {
		for (const string &file_name : iter.second) {
			cout << "Downloaded file: " << file_name << endl;
		}
	}*/
	index_all(downloaded_files);
	sort_all();
	upload_all();

	delete m_sub_system;

	deinit_aws_api();

}

template<class TemplateIndexer>
map<int, vector<string>> CCIndexRunner<TemplateIndexer>::download_all(size_t limit) {

	string warc_paths_url = string("crawl-data/") + m_cc_batch + "/warc.paths.gz";
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

		string output_file = TemplateIndexer::get_downloaded_file_name(shard, id);
		shard_files[shard].push_back(output_file);

		if (limit && id >= limit) break;
		id++;
	}

	for(auto && result: results) {
		result.get();
	}

	return shard_files;
}

template<class TemplateIndexer>
void CCIndexRunner<TemplateIndexer>::index_all(const map<int, vector<string>> &files) {
	Profiler split_profiler("Split files");

	vector<thread> threads;
	for (const auto &iter : files) {
		thread th (&CCIndexRunner::run_indexer_thread, this, iter.second, iter.first);
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

template<class TemplateIndexer>
void CCIndexRunner<TemplateIndexer>::sort_all() {
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
		thread th (&CCIndexRunner::run_sorter_thread, this, chunk);
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

template<class TemplateIndexer>
void CCIndexRunner<TemplateIndexer>::upload_all() {
	// Uploading

	Profiler profile("Uploading");

	ThreadPool pool(CC_NUM_THREADS_UPLOADING);
	std::vector<std::future<string>> results;

	int idx = 0;
	size_t word_size = m_sub_system->words().size();
	for (const string &word : m_sub_system->words()) {

		results.emplace_back(
			pool.enqueue([this, word, idx, word_size] {
				upload_results_thread(word);
				return word + " done " + to_string(idx) + " out of " + to_string(word_size);
			})
		);
		idx++;
	}

	for (auto && result: results) {
		cout << result.get() << endl;
	}
}

template<class TemplateIndexer>
string CCIndexRunner<TemplateIndexer>::run_download_thread(const string &warc_path, int shard, int id) {
	const string bucket = "alexandria-cc-output";
	TemplateIndexer indexer(m_sub_system);
	indexer.download(bucket, warc_path, shard, id);
	return "";
}

template<class TemplateIndexer>
void CCIndexRunner<TemplateIndexer>::run_indexer_thread(const vector<string> &file_names, int shard) {

	TemplateIndexer indexer(m_sub_system);
	indexer.index(file_names, shard);
}

template<class TemplateIndexer>
void CCIndexRunner<TemplateIndexer>::run_sorter_thread(const vector<string> &chunk) {

	try {
		TemplateIndexer indexer(m_sub_system);
		indexer.sorter(chunk);
	} catch (runtime_error &error) {
		cout << error.what() << endl;
	}
}

template<class TemplateIndexer>
void CCIndexRunner<TemplateIndexer>::upload_results_thread(const string &word) {

	try {
		TemplateIndexer indexer(m_sub_system);
		indexer.upload(m_cc_batch, word, 3);
	} catch (runtime_error &error) {
		cout << error.what() << endl;
	}
}
