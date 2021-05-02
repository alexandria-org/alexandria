
#include "TsvFile.h"

TsvFile::TsvFile(const string &file_name) :
	m_file(file_name)
{
	m_file.seekg(0, m_file.end);
	m_file_size = m_file.tellg();
}

TsvFile::~TsvFile() {
	m_file.close();
}

string TsvFile::find(const string &key) {
	size_t pos = binary_find_position(m_file_size, 0, key);
	if (pos == string::npos) {
		return "";
	}

	m_file.seekg(pos, m_file.beg);

	string line;
	getline(m_file, line);

	return line;
}

map<string, string> TsvFile::find_all(const set<string> &keys) {
	size_t pos = 0;
	map<string, string> result;
	string line;
	for (const string &key : keys) {
		pos = binary_find_position(m_file_size, pos, key);
		if (pos != string::npos) {
			m_file.seekg(pos, m_file.beg);
			getline(m_file, line);
			result[key] = line;
		} else {
			// Key not found, ignore.
		}
	}

	return result;
}

size_t TsvFile::read_column_into(int column, set<string> &container) {
	m_file.seekg(0, m_file.beg);

	string line;
	size_t rows_read = 0;
	while (getline(m_file, line)) {
		stringstream ss(line);
		string col;
		ss >> col;
		container.insert(col);
		rows_read++;
	}

	return rows_read;
}

size_t TsvFile::read_column_into(int column, vector<string> &container) {
	m_file.seekg(0, m_file.beg);

	string line;
	size_t rows_read = 0;
	while (getline(m_file, line)) {
		stringstream ss(line);
		string col;
		ss >> col;
		container.push_back(col);
		rows_read++;
	}

	return rows_read;
}

size_t TsvFile::binary_find_position(size_t file_size, size_t offset, const string &key) {

	string line;

	if (file_size - offset < 750) {
		// Make linear search.
		m_file.seekg(offset, m_file.beg);
		size_t bytes_read = 0;
		while (getline(m_file, line) && bytes_read <= file_size - offset) {
			bytes_read += (line.size() + 1u);
			if (line.find(key + "\t") == 0) {
				return (size_t)m_file.tellg() - (line.size() + 1u);
			}
		}

		return -1;
	}

	size_t pivot_len_1 = (file_size - offset) / 2;
	size_t pivot_len_2 = (file_size - offset) - pivot_len_1;
	size_t pivot = offset + pivot_len_1;

	// Get key at pivot.
	m_file.seekg(pivot, m_file.beg);

	getline(m_file, line);
	getline(m_file, line);
	string pivot_key = line.substr(0, line.find("\t"));

	if (key < pivot_key) {
		return binary_find_position(offset + pivot_len_1, offset, key);
	} else if (key > pivot_key) {
		return binary_find_position(file_size, pivot, key);
	}

	return (size_t)m_file.tellg() - (line.size() + 1u);
}