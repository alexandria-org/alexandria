
#include "FullTextIndexerRunner.h"
#include "FullTextIndexer.h"
#include <math.h>
#include "system/Logger.h"

FullTextIndexerRunner::FullTextIndexerRunner(const string &cc_batch)
: m_cc_batch(cc_batch)
{
	m_sub_system = new SubSystem();
}

FullTextIndexerRunner::~FullTextIndexerRunner() {
	delete m_sub_system;
}

void FullTextIndexerRunner::run() {

	// Make searches.
	/*FullTextIndex fti("main_index");
	fti.wait_for_start();

	HashTable hash_table;
	hash_table.wait_for_start();

	Profiler profiler1("Make Search 1");
	vector<FullTextResult> result = fti.search_phrase("Rehabilitation Centers Singing River");
	profiler1.stop();

	Profiler profiler2("Fetch urls 1");
	for (FullTextResult &res : result) {
		cout << "found ID: " << res.m_value << endl;
		cout << "found url: " << hash_table.find(res.m_value) << endl;
	}
	profiler2.stop();

	Profiler profiler3("Make Search 2");
	result = fti.search_phrase("Rehabilitation Centers Singing River");
	profiler3.stop();

	Profiler profiler4("Fetch urls 2");
	for (FullTextResult &res : result) {
		cout << "found ID: " << res.m_value << endl;
		cout << "found url: " << hash_table.find(res.m_value) << endl;
	}
	profiler4.stop();

	return;*/

	string warc_paths_url = string("crawl-data/") + m_cc_batch + "/warc.paths.gz";
	TsvFileS3 warc_paths_file(m_sub_system->s3_client(), "commoncrawl", warc_paths_url);

	vector<string> warc_paths_raw, warc_paths;
	warc_paths_file.read_column_into(0, warc_paths_raw);

	for (size_t i = 0; i < 10000; i++) {
		warc_paths.push_back(warc_paths_raw[i]);
	}

	vector<vector<string>> warc_path_chunks;
	vector_chunk(warc_paths, ceil((float)warc_paths.size() / FT_NUM_THREADS_INDEXING), warc_path_chunks);

	ThreadPool pool(FT_NUM_THREADS_INDEXING);
	std::vector<std::future<string>> results;

	map<int, vector<string>> shard_files;
	int id = 1;
	for (const vector<string> &warc_paths_chunk : warc_path_chunks) {

		results.emplace_back(
			pool.enqueue([this, warc_paths_chunk, id] {
				return run_index_thread(warc_paths_chunk, id);
			})
		);

		id++;

	}

	for(auto && result: results) {
		result.get();
	}

	cout << "Done indexing.. Sleeping 100..." << endl;

	// Loop over shards and merge them.
	for (size_t shard_id = 0; shard_id < FT_NUM_SHARDS; shard_id++) {
		const string file_name = "/mnt/"+(to_string(shard_id % 8))+"/output/precache_" + to_string(shard_id) + ".fti";
		FullTextShardBuilder shard(file_name);

		shard.merge("main_index", shard_id);
	}

	LogInfo("Done!");

}

string FullTextIndexerRunner::run_index_thread(const vector<string> &warc_paths, int id) {

	vector<HashTableShardBuilder *> shard_builders;
	for (size_t i = 0; i < HT_NUM_SHARDS; i++) {
		shard_builders.push_back(new HashTableShardBuilder(i));
	}

	FullTextIndexer indexer(id);
	size_t idx = 1;
	for (const string &raw_warc_path : warc_paths) {
		stringstream stream;

		string warc_path = raw_warc_path;
		warc_path.replace(warc_path.find(".warc.gz"), 8, ".gz");

		if (download_file("alexandria-cc-output", warc_path, stream) == 0) {
			indexer.add_stream(shard_builders, stream, {1, 2, 3, 4}, {1, 1, 1, 1});
			if (indexer.should_write_cache()) {
				m_write_mutex.lock();
				indexer.write_cache();
				m_write_mutex.unlock();
			}
		}

		for (size_t i = 0; i < HT_NUM_SHARDS; i++) {
			if (shard_builders[i]->full()) {
				m_hash_table_mutexes[i].lock();
				shard_builders[i]->write();
				m_hash_table_mutexes[i].unlock();
			}
		}

		LogInfo("Done " + to_string(idx) + " out of " + to_string(warc_paths.size()));

		idx++;
	}
	m_write_mutex.lock();
	indexer.write_cache();
	m_write_mutex.unlock();

	for (size_t i = 0; i < HT_NUM_SHARDS; i++) {
		m_hash_table_mutexes[i].lock();
		shard_builders[i]->write();
		m_hash_table_mutexes[i].unlock();
	}

	for (HashTableShardBuilder *shard_builder : shard_builders) {
		delete shard_builder;
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
