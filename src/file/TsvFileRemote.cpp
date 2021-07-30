
#include "TsvFileRemote.h"

TsvFileRemote::TsvFileRemote(const string &file_name) {
	// Check if the file exists.

	m_file_name = file_name;

	ifstream infile(get_path());

	if (!infile.good()) {
		download_file();
	}

	set_file_name(get_path());
}

TsvFileRemote::~TsvFileRemote() {
	
}

string TsvFileRemote::get_path() const {
	return string(TSV_FILE_DESTINATION) + "/" + m_file_name;
}

int TsvFileRemote::download_file() {

	if (m_file_name.find(".gz") == m_file_name.size() - 3) {
		m_is_gzipped = true;
	} else {
		m_is_gzipped = false;
	}

	cout << "Downloading file with key: " << m_file_name << endl;

	create_directory();
	ofstream outfile(get_path(), ios::trunc);

	int error;
	if (outfile.good()) {
		if (m_is_gzipped) {
			Transfer::gz_file_to_stream(m_file_name, outfile, error);
		} else {
			Transfer::file_to_stream(m_file_name, outfile, error);
		}
	} else {
		return CC_ERROR;
	}

	cout << "Done downloading file with key: " << m_file_name << endl;

	return CC_OK;
}

void TsvFileRemote::create_directory() {
	boost::filesystem::path path(get_path());
	boost::filesystem::create_directories(path.parent_path());
}
