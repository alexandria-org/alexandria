
#include "FullTextIndexerRunner.h"
#include "FullTextIndexer.h"
#include <math.h>
#include "system/Logger.h"

FullTextIndexerRunner::FullTextIndexerRunner(const string &db_name, const string &cc_batch)
: m_cc_batch(cc_batch), m_db_name(db_name)
{
	m_sub_system = new SubSystem();
}

FullTextIndexerRunner::~FullTextIndexerRunner() {
	delete m_sub_system;
}

void FullTextIndexerRunner::run() {

	truncate();

	string warc_paths_url = string("crawl-data/") + m_cc_batch + "/warc.paths.gz";
	TsvFileS3 warc_paths_file(m_sub_system->s3_client(), "commoncrawl", warc_paths_url);

	vector<string> warc_paths_raw, warc_paths;
	warc_paths_file.read_column_into(0, warc_paths_raw);

	size_t num = 0;
	for (const string &path : warc_paths_raw) {
		warc_paths.push_back(path);
		num++;
		//if (num >= 10000) break;
		if (num >= 100) break;
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

	merge();
	sort();
	//upload();

}

void FullTextIndexerRunner::merge() {
	LogInfo("Merging...");

	const size_t merge_batch_size = 500;

	ThreadPool merge_pool(FT_NUM_THREADS_MERGING);
	std::vector<std::future<string>> merge_results;

	// Loop over shards and merge them.
	for (size_t shard_id = 0; shard_id < FT_NUM_SHARDS; ) {

		while (shard_id < FT_NUM_SHARDS && merge_results.size() < merge_batch_size) {

			merge_results.emplace_back(
				merge_pool.enqueue([this, shard_id] {
					return run_merge_thread(shard_id);
				})
			);

			shard_id++;

		}

		for (auto && result: merge_results) {
			result.get();
		}
		merge_results.clear();
	}
}

void FullTextIndexerRunner::sort() {
	LogInfo("Sorting...");

	// Loop over hash table shards and merge them.
	for (size_t shard_id = 0; shard_id < HT_NUM_SHARDS; shard_id++) {
		HashTableShard *shard = new HashTableShard(m_db_name, shard_id);
		shard->sort();
		delete shard;
	}
}

void FullTextIndexerRunner::upload() {
	LogInfo("Uploading...");

	FullTextIndex fti(m_db_name);
	fti.upload(m_sub_system);

	HashTable hash_table(m_db_name);
	hash_table.upload(m_sub_system);
}

void FullTextIndexerRunner::index_text(const string &text) {

	vector<HashTableShardBuilder *> shard_builders;
	for (size_t i = 0; i < HT_NUM_SHARDS; i++) {
		shard_builders.push_back(new HashTableShardBuilder(m_db_name, i));
	}

	stringstream ss(text);

	FullTextIndexer indexer(1, m_db_name, m_sub_system);
	indexer.add_stream(shard_builders, ss, {1, 2, 3, 4}, {1, 1, 1, 1});
	indexer.write_cache(m_full_text_mutexes);
	indexer.flush_cache(m_full_text_mutexes);

	for (size_t i = 0; i < HT_NUM_SHARDS; i++) {
		shard_builders[i]->write();
	}

	indexer.write_url_to_domain();

	for (HashTableShardBuilder *shard_builder : shard_builders) {
		delete shard_builder;
	}
}

void FullTextIndexerRunner::index_text(const string &key, const string &text, uint32_t score) {

	vector<HashTableShardBuilder *> shard_builders;
	for (size_t i = 0; i < HT_NUM_SHARDS; i++) {
		shard_builders.push_back(new HashTableShardBuilder(m_db_name, i));
	}

	FullTextIndexer indexer(1, m_db_name, m_sub_system);
	indexer.add_text(shard_builders, key, text, score);
	indexer.write_cache(m_full_text_mutexes);
	indexer.flush_cache(m_full_text_mutexes);

	for (size_t i = 0; i < HT_NUM_SHARDS; i++) {
		shard_builders[i]->write();
	}

	indexer.write_url_to_domain();

	for (HashTableShardBuilder *shard_builder : shard_builders) {
		delete shard_builder;
	}
}

void FullTextIndexerRunner::index_warc_path(const string warc_path) {

	vector<HashTableShardBuilder *> shard_builders;
	for (size_t i = 0; i < HT_NUM_SHARDS; i++) {
		shard_builders.push_back(new HashTableShardBuilder(m_db_name, i));
	}

	stringstream stream;
	FullTextIndexer indexer(1, m_db_name, m_sub_system);
	if (download_file("alexandria-cc-output", warc_path, stream) == 0) {
		indexer.add_stream(shard_builders, stream, {1, 2, 3, 4}, {1, 1, 1, 1});
		indexer.write_cache(m_full_text_mutexes);
		indexer.flush_cache(m_full_text_mutexes);
	}

	for (size_t i = 0; i < HT_NUM_SHARDS; i++) {
		shard_builders[i]->write();
	}

	indexer.write_url_to_domain();

	for (HashTableShardBuilder *shard_builder : shard_builders) {
		delete shard_builder;
	}

}

void FullTextIndexerRunner::index_stream(ifstream &infile) {

	vector<HashTableShardBuilder *> shard_builders;
	for (size_t i = 0; i < HT_NUM_SHARDS; i++) {
		shard_builders.push_back(new HashTableShardBuilder(m_db_name, i));
	}

	FullTextIndexer indexer(1, m_db_name, m_sub_system);
	indexer.add_stream(shard_builders, infile, {1, 2, 3, 4}, {1, 1, 1, 1});
	indexer.flush_cache(m_full_text_mutexes);

	for (size_t i = 0; i < HT_NUM_SHARDS; i++) {
		shard_builders[i]->write();
	}

	indexer.write_url_to_domain();

	for (HashTableShardBuilder *shard_builder : shard_builders) {
		delete shard_builder;
	}
}

void FullTextIndexerRunner::truncate() {
	for (size_t shard_id = 0; shard_id < FT_NUM_SHARDS; shard_id++) {
		FullTextShardBuilder *shard_builder = new FullTextShardBuilder(m_db_name, shard_id);
		shard_builder->truncate();
		delete shard_builder;
	}

	for (size_t bucket_id = 1; bucket_id < 8; bucket_id++) {
		const string file_name = "/mnt/"+to_string(bucket_id)+"/full_text/url_to_domain_"+m_db_name+".fti";
		ofstream outfile(file_name, ios::binary | ios::trunc);
		outfile.close();
	}

	HashTable hash_table(m_db_name);
	hash_table.truncate();
}

string FullTextIndexerRunner::run_index_thread(const vector<string> &warc_paths, int id) {

	vector<HashTableShardBuilder *> shard_builders;
	for (size_t i = 0; i < HT_NUM_SHARDS; i++) {
		shard_builders.push_back(new HashTableShardBuilder(m_db_name, i));
	}

	FullTextIndexer indexer(id, m_db_name, m_sub_system);
	size_t idx = 1;
	for (const string &raw_warc_path : warc_paths) {
		stringstream stream;

		string warc_path = raw_warc_path;
		warc_path.replace(warc_path.find(".warc.gz"), 8, ".gz");

		if (download_file("alexandria-cc-output", warc_path, stream) == 0) {
			indexer.add_stream(shard_builders, stream, {1, 2, 3, 4}, {1, 1, 1, 1});
			indexer.write_cache(m_full_text_mutexes);
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
	indexer.flush_cache(m_full_text_mutexes);

	for (size_t i = 0; i < HT_NUM_SHARDS; i++) {
		m_hash_table_mutexes[i].lock();
		shard_builders[i]->write();
		m_hash_table_mutexes[i].unlock();
	}

	m_write_url_to_domain_mutex.lock();
	indexer.write_url_to_domain();
	m_write_url_to_domain_mutex.unlock();

	for (HashTableShardBuilder *shard_builder : shard_builders) {
		delete shard_builder;
	}

	return "";
}

string FullTextIndexerRunner::run_merge_thread(size_t shard_id) {

	FullTextShardBuilder shard(m_db_name, shard_id);
	shard.merge();

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

