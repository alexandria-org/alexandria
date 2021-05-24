
#pragma once

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string.hpp>

#include "SubSystem.h"
#include "ThreadPool.h"

using namespace std;

#define CC_NUM_THREADS_MERGING 10000

class CCIndexMerger {

public:

	CCIndexMerger(const string &cc_batch_source, const string &cc_batch_destination);
	~CCIndexMerger();

	void run_all();
	void run_all(size_t limit);
	void run_merge_thread(const string &word);

	string download_file(const string &file_name);
	string merge_file(const string &file1, const string &file2, vector<size_t> merge_by, size_t sort_by, size_t limit);
	void upload_file(const string &file_name, const string &data, int retries);

private:

	SubSystem *m_sub_system;

	hash<string> m_hasher;
	string m_cc_batch_source;
	string m_cc_batch_destination;

};
