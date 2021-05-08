
#include "TsvFile.h"
#include <exception>

TsvFile::TsvFile() {

}

TsvFile::TsvFile(const string &file_name) {
	set_file_name(file_name);
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

size_t TsvFile::find_first_position(const string &key) {
	m_file.clear();
	m_file.seekg(0, m_file.beg);
	const size_t pos = binary_find_position(m_file_size, 0, key);
	if (pos == string::npos) return string::npos;
	// pos is the position of one item. but we need the first one.
	size_t jump = 1000;
	while (pos > jump) {
		m_file.seekg(pos - jump, m_file.beg);
		// read next line.
		string line;
		getline(m_file, line);
		getline(m_file, line);
		string jump_key = line.substr(0, line.find("\t"));
		if (jump_key < key) {
			// We jamp too far.
			break;
		}
		jump = jump << 1;
	}
	if (pos < jump) jump = pos;

	// The first occurance is between pos - jump and pos - (jump/2)
	// Linear search.
	m_file.seekg(pos - jump, m_file.beg);
	string line;
	if (pos > jump) {
		getline(m_file, line);
	}
	while (getline(m_file, line)) {
		string jump_key = line.substr(0, line.find("\t"));
		if (jump_key == key) {
			return (size_t)m_file.tellg() - (line.size() + 1u);
		}
	}
	return string::npos;
}

size_t TsvFile::find_last_position(const string &key) {
	m_file.clear();
	m_file.seekg(0, m_file.beg);
	const size_t pos = binary_find_position(m_file_size, 0, key);
	if (pos == string::npos) return string::npos;
	// pos is the position of one item. but we need the last one.
	size_t jump = 1000;
	while (pos + jump < m_file_size) {
		m_file.seekg(pos + jump, m_file.beg);
		// read next line.
		string line;
		getline(m_file, line);
		getline(m_file, line);
		string jump_key = line.substr(0, line.find("\t"));
		if (jump_key > key) {
			// We jamp too far.
			break;
		}
		jump = jump << 1;
	}
	jump = jump >> 1;
	if (pos + jump > m_file_size) {
		jump = 0;
	}

	// The first occurance is between pos - jump and pos - (jump/2)
	// Linear search.
	m_file.seekg(pos + jump, m_file.beg);
	size_t ret_pos = pos + jump;
	string line;
	getline(m_file, line);
	size_t last_line_length = line.size() + 1u;
	ret_pos += line.size() + 1u;
	while (getline(m_file, line)) {
		string jump_key = line.substr(0, line.find("\t"));
		if (jump_key > key) {
			return ret_pos - last_line_length;
		}
		ret_pos += line.size() + 1u;
		last_line_length = line.size() + 1u;
	}
	return ret_pos - last_line_length;
}

size_t TsvFile::find_next_position(const string &key) {
	m_file.clear();
	m_file.seekg(0, m_file.beg);
	const size_t pos = binary_find_position_any(m_file_size, 0, key);

	// pos is the position of one item. but we need the last one.
	size_t jump = 1000;
	while (pos + jump < m_file_size) {
		m_file.seekg(pos + jump, m_file.beg);
		// read next line.
		string line;
		getline(m_file, line);
		getline(m_file, line);
		string jump_key = line.substr(0, line.find("\t"));
		if (jump_key > key) {
			// We jamp too far.
			break;
		}
		jump = jump << 1;
	}
	jump = jump >> 1;
	if (pos + jump > m_file_size) {
		jump = 0;
	}

	// The first occurance is between pos - jump and pos - (jump/2)
	// Linear search.
	m_file.seekg(pos + jump, m_file.beg);
	size_t ret_pos = pos + jump;
	string line;
	getline(m_file, line);
	ret_pos += line.size() + 1u;
	while (getline(m_file, line)) {
		string jump_key = line.substr(0, line.find("\t"));
		if (jump_key > key) {
			return ret_pos;
		}
		ret_pos += line.size() + 1u;
	}
	return m_file_size;
}

map<string, string> TsvFile::find_all(const set<string> &keys) {
	m_file.clear();
	m_file.seekg(0, m_file.beg);
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
	m_file.clear();
	m_file.seekg(0, m_file.beg);

	if (!m_file.is_open()) {
		throw runtime_error("File is not open any more: " + m_file_name);
	}

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

size_t TsvFile::size() const {
	return m_file_size;
}

bool TsvFile::eof() const {
	return m_file.eof();
}

string TsvFile::get_line() {
	string line;
	getline(m_file, line);
	return line;
}

size_t TsvFile::read_column_into(int column, vector<string> &container) {
	m_file.clear();
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

		return string::npos;
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

size_t TsvFile::binary_find_position_any(size_t file_size, size_t offset, const string &key) {

	string line;

	if (file_size - offset < 750) {
		// Make linear search.
		m_file.seekg(offset, m_file.beg);
		size_t bytes_read = 0;
		while (getline(m_file, line) && bytes_read <= file_size - offset) {
			bytes_read += (line.size() + 1u);
			const string key = line.substr(0, line.find("\t"));
			if (key >= key) {
				return (size_t)m_file.tellg() - (line.size() + 1u);
			}
		}

		return m_file_size;
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

void TsvFile::set_file_name(const string &file_name) {

	m_file_name = file_name;
	m_original_file_name = file_name;

	m_file.open(m_file_name);

	if (!m_file.is_open()) {
		throw runtime_error("Could not open file: " + m_file_name + " error: " + strerror(errno));
	}

	m_file.seekg(0, m_file.end);
	m_file_size = m_file.tellg();
	m_file.seekg(0, m_file.beg);
}