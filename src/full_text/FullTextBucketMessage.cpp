
#include "FullTextBucketMessage.h"
#include "system/Logger.h"

FullTextBucketMessage::FullTextBucketMessage() {
	m_data_size = 0;
}

FullTextBucketMessage::FullTextBucketMessage(const FullTextBucketMessage &message) {

	m_message_type = message.m_message_type;
	m_key = message.m_key;
	m_value = message.m_value;
	m_score = message.m_score;

	m_data_size = message.m_data_size;
	if (m_data_size) {
		m_data = new char[m_data_size];
		memcpy(m_data, message.m_data, m_data_size);
	}
}

FullTextBucketMessage &FullTextBucketMessage::operator = (const FullTextBucketMessage &message) {
	m_message_type = message.m_message_type;
	m_key = message.m_key;
	m_value = message.m_value;
	m_score = message.m_score;

	m_data_size = message.m_data_size;
	if (m_data_size) {
		m_data = new char[m_data_size];
		memcpy(m_data, message.m_data, m_data_size);
	}

	return *this;
}

FullTextBucketMessage::~FullTextBucketMessage() {
	if (m_data_size) {
		delete m_data;
	}
}

void FullTextBucketMessage::allocate_data() {
	if (m_data_size) {
		m_data = new char[m_data_size];
	}
}

char *FullTextBucketMessage::data() {
	return m_data;
}

vector<FullTextResult> FullTextBucketMessage::result_vector() {
	vector<FullTextResult> results;
	for (size_t i = 0; i < m_data_size; i += sizeof(FullTextResult)) {
		results.emplace_back(*((FullTextResult *)(m_data + i)));
	}
	return results;
}

void FullTextBucketMessage::store_file_data(const string &file_name, const vector<size_t> &cols,
	const vector<uint32_t> &scores) {

	m_filename_len = file_name.length();
	m_file_cols_len = cols.size() * sizeof(size_t);
	m_file_scores_len = scores.size() * sizeof(size_t);

	m_data_size = m_filename_len + m_file_cols_len + m_file_scores_len;
	allocate_data();

	size_t offset = 0;
	memcpy(&m_data[offset], file_name.data(), m_filename_len);
	offset += m_filename_len;

	memcpy(&m_data[offset], cols.data(), m_file_cols_len);
	offset += m_file_cols_len;

	memcpy(&m_data[offset], scores.data(), m_file_scores_len);
}

void FullTextBucketMessage::read_file_data(string &file_name, vector<size_t> &cols, vector<uint32_t> &scores) {
	file_name = string(&m_data[0], m_filename_len);

	const char *col_data = &m_data[m_filename_len];
	for (size_t i = 0; i < m_file_cols_len; i += sizeof(size_t)) {
		cols.push_back(*((size_t *)(col_data + i)));
	}

	const char *score_data = &m_data[m_filename_len + m_file_cols_len];
	for (size_t i = 0; i < m_file_scores_len; i += sizeof(uint32_t)) {
		scores.push_back(*((uint32_t *)(score_data + i)));
	}
}

vector<FullTextResult> FullTextBucketMessage::debug() {
	vector<FullTextResult> results;
	for (size_t i = 0; i < m_data_size; i += sizeof(FullTextResult)) {
		FullTextResult res(*((FullTextResult *)(m_data + i)));

		if (res.m_value == 0) {
			LogInfo("Value is zero no 2!");
			throw error("Value is zero...");
		}
		results.push_back(res);
	}
	return results;
}
