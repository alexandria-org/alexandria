
#pragma once

#include "SubSystem.h"
#include "ThreadPool.h"
#include "CCLinkIndex.h"
#include "CCUrlIndex.h"

using namespace std;

#define CC_NUM_THREADS_DOWNLOADING 128
#define CC_NUM_THREADS_UPLOADING 10000
#define CC_NUM_THREADS_SORTING 32
#define CC_NUM_THREADS_INDEXING 32

template<class TemplateIndexer>
class CCIndexRunner {

public:

	CCIndexRunner(const string &cc_batch);
	~CCIndexRunner();

	void run_all();
	void run_all(size_t limit);
	map<int, vector<string>> download_all(size_t limit);
	void index_all(const map<int, vector<string>> &files);
	void sort_all();
	void upload_all();

	string run_download_thread(const string &warc_path, int shard, int id);
	void run_indexer_thread(const vector<string> &file_names, int shard);
	void run_sorter_thread(const vector<string> &chunk);
	void upload_results_thread(const string &word);

private:

	SubSystem *m_sub_system;
	string m_cc_batch;

};

template class CCIndexRunner<CCLinkIndex>;
template class CCIndexRunner<CCUrlIndex>;
