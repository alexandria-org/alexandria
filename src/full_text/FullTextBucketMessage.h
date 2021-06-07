
#pragma once

#include <iostream>
#include <string.h>
#include <vector>

#include "FullTextResult.h"

using namespace std;

class FullTextBucketMessage {

public:

	FullTextBucketMessage();
	FullTextBucketMessage(const FullTextBucketMessage &message);
	~FullTextBucketMessage();

	FullTextBucketMessage &operator = (const FullTextBucketMessage &message);

	void allocate_data();

	char *data();
	const char *data() const { return m_data; };
	vector<FullTextResult> result_vector();
	void store_file_data(const string &file_name, const vector<size_t> &cols, const vector<uint32_t> &scores);
	void read_file_data(string &file_name, vector<size_t> &cols, vector<uint32_t> &scores);

	size_t m_message_type;
	size_t m_data_size;
	uint64_t m_key;
	uint64_t m_value;
	uint32_t m_score;
	size_t m_size_response;

	size_t m_filename_len;
	size_t m_file_cols_len;
	size_t m_file_scores_len;

	vector<FullTextResult> debug();

private:
	bool m_did_allocate;
	char *m_data = NULL;

};
