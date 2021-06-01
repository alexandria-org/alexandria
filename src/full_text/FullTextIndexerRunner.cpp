
#include "FullTextIndexerRunner.h"
#include "FullTextIndexer.h"
#include <math.h>

FullTextIndexerRunner::FullTextIndexerRunner(const string &cc_batch)
: m_cc_batch(cc_batch)
{

}

FullTextIndexerRunner::~FullTextIndexerRunner() {

}

void FullTextIndexerRunner::run() {

	init_aws_api();

	Aws::S3::S3Client s3_client = get_s3_client();
	m_sub_system = new SubSystem(s3_client);

	string warc_paths_url = string("crawl-data/") + m_cc_batch + "/warc.paths.gz";
	TsvFileS3 warc_paths_file(m_sub_system->s3_client(), "commoncrawl", warc_paths_url);

	vector<string> warc_paths;
	warc_paths_file.read_column_into(0, warc_paths);

	vector<vector<string>> warc_path_chunks;
	vector_chunk(warc_paths, ceil((float)warc_paths.size() / FT_NUM_THREADS_INDEXING), warc_path_chunks);

	ThreadPool pool(FT_NUM_THREADS_INDEXING);
	std::vector<std::future<string>> results;

	int id = 1;
	map<int, vector<string>> shard_files;
	for (const vector<string> &warc_paths_chunk : warc_path_chunks) {

		results.emplace_back(
			pool.enqueue([this, warc_paths_chunk] {
				return run_index_thread(warc_paths_chunk);
			})
		);

		id++;
		break;
	}

	for(auto && result: results) {
		result.get();
	}

	// Loop over shards and merge them.
	for (size_t shard_id = 0; shard_id < FT_NUM_SHARDS; shard_id++) {
		const string file_name = "/mnt/"+(to_string(shard_id % 8))+"/output/precache_" + to_string(shard_id) + ".fti";
		FullTextShard shard(file_name);

		shard.merge_precache();
	}

	deinit_aws_api();
}

string FullTextIndexerRunner::run_index_thread(const vector<string> &warc_paths) {

	FullTextIndexer indexer;
	for (const string &raw_warc_path : warc_paths) {
		stringstream stream;

		string warc_path = raw_warc_path;
		warc_path.replace(warc_path.find(".warc.gz"), 8, ".gz");

		if (download_file("alexandria-cc-output", warc_path, stream) == 0) {
			indexer.add_stream(stream, {1, 2, 3, 4}, {1, 1, 1, 1});
			if (indexer.should_write_cache()) {
				m_write_mutex.lock();
				indexer.write_cache();
				m_write_mutex.unlock();
			}
		}
	}

	return "";
}

int FullTextIndexerRunner::download_file(const string &bucket, const string &key, stringstream &stream) {

	Aws::S3::Model::GetObjectRequest request;
	cout << "Downloading " << bucket << " key: " << key << endl;
	request.SetBucket(bucket);
	request.SetKey(key);

	auto outcome = m_sub_system->s3_client().GetObject(request);

	if (outcome.IsSuccess()) {

		auto &input_stream = outcome.GetResultWithOwnership().GetBody();

		filtering_istream decompress_stream;
		decompress_stream.push(gzip_decompressor());
		decompress_stream.push(input_stream);

		stream << decompress_stream.rdbuf();

		return 0;
	}

	return 1;
}