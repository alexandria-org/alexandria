
#include <iterator>
#include <aws/core/Aws.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/core/auth/AWSCredentialsProvider.h>

#include <iostream>
#include <memory>
#include <unistd.h>

#include "CCIndexer.h"
#include "SubSystem.h"
#include "ThreadPool.h"
#include <cmath>

#define CC_NUM_THREADS_DOWNLOADING 128
#define CC_NUM_THREADS_UPLOADING 512
#define CC_NUM_THREADS_INDEXING 32

using namespace std;
using namespace Aws::Utils::Json;

namespace io = boost::iostreams;

std::function<std::shared_ptr<Aws::Utils::Logging::LogSystemInterface>()> get_logger_factory() {
	return [] {
		return Aws::MakeShared<Aws::Utils::Logging::ConsoleLogSystem>(
			"console_logger", Aws::Utils::Logging::LogLevel::Error);
	};
}

void run_indexer(const SubSystem *sub_system, const string &warc_path, int shard, int id) {
	const string bucket = "alexandria-cc-output";
	CCIndexer indexer(sub_system);
	indexer.run(bucket, warc_path, shard, id);
}

void run_indexer2(const SubSystem *sub_system, const vector<string> &chunk, const vector<string> &input_files,
		int shard) {

	CCIndexer indexer(sub_system);
	indexer.index(chunk, input_files, shard);
}

void run_sorter(const SubSystem *sub_system, const vector<string> &chunk, int shard) {

	CCIndexer indexer(sub_system);
	indexer.sorter(chunk, shard);
}

void upload_results(SubSystem *sub_system, const string &word, int retries) {

	Aws::S3::Model::PutObjectRequest request;
	request.SetBucket("alexandria-index");
	string key = "CC-MAIN-2021-10/index_" + word + ".tsv.gz";
	request.SetKey(key);

	ifstream infile;
	for (int shard = 0; shard < 8; shard++) {
		infile.open("/mnt/" + to_string(shard) + "/output/index_" + word + ".tsv");
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
			upload_results(sub_system, word, retries - 1);
		}
	}
}

int main(int argc, const char **argv) {

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

		thread th(run_indexer, sub_system, warc_path, shard, id);

		threads.push_back(move(th));

		if (threads.size() == CC_NUM_THREADS_DOWNLOADING) {
			// join threads.
			for (thread &_th : threads) {
				_th.join();
			}
			threads.clear();
		}
		
		string output_file = "/mnt/"+to_string(shard)+"/output_"+to_string(id)+".tsv";
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
		thread th (run_indexer2, sub_system, chunk, input_files, shard);
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
		thread th (run_sorter, sub_system, chunk, shard);
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
				upload_results(sub_system, word, 3);
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

	return 0;
}

