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

#include <mutex>
#include <fstream>
#include <iostream>

#define LogInfo(msg) Logger::log("info", __FILE__, __LINE__, msg)
#define LogError(msg) Logger::log("error", __FILE__, __LINE__, msg)

#define error(msg) LoggedException(msg, string(__FILE__), __LINE__)

using namespace std;

class Logger {

public:

	static string m_filename;

	Logger();
	~Logger();

	void verbose(bool verbose);
	void reopen();
	string timestamp() const;
	string format(const string &type, const string &file, int line, const string &message, const string &meta) const;
	void log_message(const string &type, const string &file, int line, const string &message, const string &meta);
	void log_string(const string &message);

	// Should be called like this: Logger::log("error", __FILE__, __LINE__, error.what());
	static void log(const string &type, const string &file, int line, const string &message);
	static void log(const string &type, const string &file, int line, const string &message, const string &meta);
	static Logger *instance();

	// Singleton, cannot be copied.
	Logger(Logger const&) = delete;
	void operator =(Logger const&) = delete;

private:
	mutex m_lock;
	ofstream m_file;
	chrono::seconds m_reopen_interval;
	chrono::system_clock::time_point m_last_reopen;
	bool m_verbose;

};

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

