
#include "LinkIndexerRunner.h"
#include "LinkIndexer.h"
#include <math.h>
#include "system/Logger.h"

LinkIndexerRunner::LinkIndexerRunner(const string &db_name, const string &cc_batch, const string &fti_name)
: m_cc_batch(cc_batch), m_db_name(db_name), m_fti_name(fti_name)
{
	m_sub_system = new SubSystem();
	m_url_to_domain = new UrlToDomain(m_fti_name);
	m_url_to_domain->read();
}

LinkIndexerRunner::~LinkIndexerRunner() {
	delete m_sub_system;
	delete m_url_to_domain;
}

void LinkIndexerRunner::run() {

	truncate();

	string warc_paths_url = string("crawl-data/") + m_cc_batch + "/warc.paths.gz";
	TsvFileS3 warc_paths_file(m_sub_system->s3_client(), "commoncrawl", warc_paths_url);

	vector<string> warc_paths_raw;
	warc_paths_file.read_column_into(0, warc_paths_raw);

	const size_t limit = 25000;
	size_t run_num = 0;
	while (warc_paths_raw.size() > 0) {

		run_num++;

		vector<string> warc_paths;
		size_t num = 0;
		while (warc_paths.size() < limit && warc_paths_raw.size() > 0) {
			warc_paths.push_back(warc_paths_raw.back());
			warc_paths_raw.pop_back();
			num++;
			if (num >= 1000) break;
		}

		//if (run_num == 1) continue;

		vector<vector<string>> warc_path_chunks;
		vector_chunk(warc_paths, ceil((float)warc_paths.size() / LI_NUM_THREADS_INDEXING), warc_path_chunks);

		ThreadPool pool(LI_NUM_THREADS_INDEXING);
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
		string line;
		cout << "continue ? ";
		cin >> line;
		merge_adjustments();
		break;
	}
	
	sort();
	//upload();

}

void LinkIndexerRunner::merge() {
	LogInfo("Merging...");
	Profiler profiler("Merging...");

	const size_t merge_batch_size = 500;

	ThreadPool merge_pool(LI_NUM_THREADS_MERGING);
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

void LinkIndexerRunner::merge_adjustments() {
	LogInfo("Merging adjustments...");
	Profiler profiler("Merging adjustments...");

	ThreadPool merge_pool(24);
	std::vector<std::future<string>> merge_results;

	FullTextIndexer *indexer = new FullTextIndexer(1, m_fti_name, m_sub_system, m_url_to_domain);

	// Loop over shards and merge them.
	for (size_t shard_id = 0; shard_id < FT_NUM_SHARDS; shard_id++) {

		merge_results.emplace_back(
			merge_pool.enqueue([this, indexer, shard_id] {
				return run_merge_adjustments_thread(indexer, shard_id);
			})
		);

	}

	for (auto && result: merge_results) {
		result.get();
	}

	delete indexer;
}

void LinkIndexerRunner::sort() {
	LogInfo("Sorting...");
	Profiler profiler("Sorting...");

	// Loop over hash table shards and merge them.
	for (size_t shard_id = 0; shard_id < HT_NUM_SHARDS; shard_id++) {
		HashTableShard *shard = new HashTableShard(m_db_name, shard_id);
		shard->sort();
		delete shard;
	}
}

void LinkIndexerRunner::upload() {
	LogInfo("Uploading...");

	LinkIndex li(m_db_name);
	li.upload(m_sub_system);

	HashTable hash_table(m_db_name);
	hash_table.upload(m_sub_system);
}

void LinkIndexerRunner::truncate() {
	for (size_t shard_id = 0; shard_id < LI_NUM_SHARDS; shard_id++) {
		FullTextShardBuilder<LinkFullTextRecord> *shard_builder =
			new FullTextShardBuilder<LinkFullTextRecord>(m_db_name, shard_id);
		shard_builder->truncate();
		delete shard_builder;
	}

	for (size_t shard_id = 0; shard_id < FT_NUM_SHARDS; shard_id++) {
		FullTextShardBuilder<FullTextRecord> *shard_builder =
			new FullTextShardBuilder<FullTextRecord>("adjustments", shard_id);
		shard_builder->truncate();
		delete shard_builder;
	}

	for (size_t shard_id = 0; shard_id < FT_NUM_SHARDS; shard_id++) {
		FullTextShardBuilder<FullTextRecord> *shard_builder =
			new FullTextShardBuilder<FullTextRecord>("domain_adjustments", shard_id);
		shard_builder->truncate();
		delete shard_builder;
	}

	for (size_t shard_id = 0; shard_id < FT_NUM_SHARDS; shard_id++) {
		FullTextShardBuilder<FullTextRecord> *shard_builder =
			new FullTextShardBuilder<FullTextRecord>(m_fti_name, shard_id);
		shard_builder->truncate_cache_files();
		delete shard_builder;
	}

	HashTable hash_table(m_db_name);
	hash_table.truncate();
}

void LinkIndexerRunner::index_stream(ifstream &infile) {

	vector<HashTableShardBuilder *> shard_builders;
	for (size_t i = 0; i < HT_NUM_SHARDS; i++) {
		shard_builders.push_back(new HashTableShardBuilder(m_db_name, i));
	}

	FullTextIndexer ft_indexer(1, m_fti_name, m_sub_system, m_url_to_domain);
	ft_indexer.read_url_to_domain();

	LinkIndexer indexer(1, m_db_name, m_sub_system, &ft_indexer);
	indexer.add_stream(shard_builders, infile);

	indexer.flush_cache(m_link_mutexes);

	for (size_t i = 0; i < HT_NUM_SHARDS; i++) {
		shard_builders[i]->write();
	}

	for (HashTableShardBuilder *shard_builder : shard_builders) {
		delete shard_builder;
	}
}

string LinkIndexerRunner::run_index_thread(const vector<string> &warc_paths, int id) {

	vector<HashTableShardBuilder *> shard_builders;
	for (size_t i = 0; i < HT_NUM_SHARDS; i++) {
		shard_builders.push_back(new HashTableShardBuilder(m_db_name, i));
	}

	FullTextIndexer ft_indexer(1, m_fti_name, m_sub_system, m_url_to_domain);

	LinkIndexer indexer(id, m_db_name, m_sub_system, &ft_indexer);
	size_t idx = 1;
	for (const string &raw_warc_path : warc_paths) {
		stringstream stream;

		string warc_path = raw_warc_path;
		warc_path.replace(warc_path.find(".warc.gz"), 8, ".links.gz");

		if (download_file("alexandria-cc-output", warc_path, stream) == 0) {
			indexer.add_stream(shard_builders, stream);
			indexer.write_cache(m_link_mutexes);
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
	indexer.flush_cache(m_link_mutexes);

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

string LinkIndexerRunner::run_merge_thread(size_t shard_id) {

	FullTextShardBuilder<FullTextRecord> adjustment_shard("adjustments", shard_id);
	adjustment_shard.merge_with_sum();

	FullTextShardBuilder<FullTextRecord> domain_adjustment_shard("domain_adjustments", shard_id);
	domain_adjustment_shard.merge_with_sum();

	FullTextShardBuilder<LinkFullTextRecord> shard(m_db_name, shard_id);
	shard.merge();

	return shard.filename();
}

string LinkIndexerRunner::run_merge_adjustments_thread(const FullTextIndexer *indexer, size_t shard_id) {

	FullTextShardBuilder<FullTextRecord> shard1(m_fti_name, shard_id);
	FullTextShardBuilder<FullTextRecord> shard2("adjustments", shard_id);
	FullTextShardBuilder<FullTextRecord> shard3("domain_adjustments", shard_id);

	shard1.merge_domain(shard2, shard3, indexer->url_to_domain());

	LogInfo("Merged " + to_string(shard_id));

	return shard1.filename();
}

int LinkIndexerRunner::download_file(const string &bucket, const string &key, stringstream &stream) {

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

