/*
 * MIT License
 *
 * Alexandria.org
 *
 * Copyright (c) 2021 Josef Cullhed, <info@alexandria.org>, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "tsv_file_remote.h"
#include "logger/logger.h"
#include "transfer/transfer.h"

#include <boost/filesystem.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

using namespace std;
using namespace boost::iostreams;

namespace file {

	tsv_file_remote::tsv_file_remote(const string &file_name) {
		// Check if the file exists.

		m_file_name = file_name;

		ifstream infile(get_path());

		if (download_file() == transfer::OK) {
			set_file_name(get_path());
		} else {
			infile.close();
		}
	}

	tsv_file_remote::~tsv_file_remote() {
		
	}

	string tsv_file_remote::get_path() const {
		return config::data_path() + "/0/" + m_file_name;
	}

	int tsv_file_remote::download_file() {

		if (m_file_name.find(".gz") == m_file_name.size() - 3) {
			m_is_gzipped = true;
		} else {
			m_is_gzipped = false;
		}

		LOG_INFO("Downloading file with key: " + m_file_name);

		create_directory();
		ofstream outfile(get_path(), ios::trunc);

		int error = transfer::ERROR;
		if (outfile.good()) {
			if (m_is_gzipped) {
				transfer::gz_file_to_stream(m_file_name, outfile, error);
			} else {
				transfer::file_to_stream(m_file_name, outfile, error);
			}

			if (error == transfer::ERROR) {
				LOG_INFO("Download failed...");
			}
		}

		LOG_INFO("Done downloading file with key: " + m_file_name);

		return error;
	}

	void tsv_file_remote::create_directory() {
		boost::filesystem::path path(get_path());
		boost::filesystem::create_directories(path.parent_path());
	}

}
