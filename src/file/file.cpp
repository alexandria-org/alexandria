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

#include "config.h"
#include "file.h"
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

namespace file {

	std::string read_test_file(const std::string &file_name) {

		std::ifstream file(config::test_data_path + file_name);
		if (file.is_open()) {
			std::string ret;
			file.seekg(0, std::ios::end);
			ret.resize(file.tellg());
			file.seekg(0, std::ios::beg);
			file.read(&ret[0], ret.size());
			file.close();
			return ret;
		}
		return "";
	}

	void rename(const std::string &old_path, const std::string &new_path) {
		boost::filesystem::rename(old_path, new_path);
	}

	void copy_file(const std::string &source, const std::string &dest) {
		std::ifstream infile(source, std::ios::binary);
		std::ofstream outfile(dest, std::ios::binary | std::ios::trunc);

		outfile << infile.rdbuf();
	}

	void delete_file(const std::string &file) {
		boost::filesystem::remove(file);
	}

	void create_directory(const std::string &path) {
		boost::filesystem::create_directories(path);
	}

	void delete_directory(const std::string &path) {
		boost::filesystem::remove_all(path);
	}

	std::string cat(const std::string &filename) {
		std::ifstream infile(filename);
		std::istreambuf_iterator<char> iter(infile), end; 
		std::string ret(iter, end);
		return ret;
	}

	void read_directory(const std::string &dirname, std::function<void(const std::string &)> cb) {

		boost::filesystem::path path(dirname);

		if (is_directory(path)) {
			boost::filesystem::directory_iterator iter(path);
			for (auto &file : boost::make_iterator_range(iter, {})) {
				cb(file.path().filename().generic_string());
			}
		}
	}

	bool directory_exists(const std::string &filename) {
		return boost::filesystem::is_directory(filename) && boost::filesystem::exists(filename);
	}

	bool file_exists(const std::string &filename) {
		std::ifstream infile(filename);
		return infile.good();
	}

}
