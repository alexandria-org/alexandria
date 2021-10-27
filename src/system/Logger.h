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

#pragma once

#include "config.h"
#include <mutex>
#include <fstream>
#include <iostream>

#define LOG_INFO(msg) Logger::log("info", __FILE__, __LINE__, msg)
#define LOG_ERROR(msg) Logger::log("error", __FILE__, __LINE__, msg)

#define error(msg) Logger::LoggedException(msg, string(__FILE__), __LINE__)

using namespace std;

namespace Logger {

	void verbose(bool verbose);
	void reopen();
	string timestamp();
	string format(const string &type, const string &file, int line, const string &message, const string &meta);
	void log_message(const string &type, const string &file, int line, const string &message, const string &meta);
	void log_string(const string &message);

	// Should be called like this: Logger::log("error", __FILE__, __LINE__, error.what());
	void log(const string &type, const string &file, int line, const string &message);
	void log(const string &type, const string &file, int line, const string &message, const string &meta);

	void start_logger_thread();
	void join_logger_thread();

	class LoggedException : public std::exception {

	public:
		LoggedException(const string &message, const string &file, int line);

		const char *what() const throw () {
			return m_formatted_message.c_str();
		}

	private:

		string m_formatted_message;
		string m_file;
		int m_line;
		string m_message;

	};

};

