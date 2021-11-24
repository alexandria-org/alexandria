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

#include "Logger.h"
#include <thread>
#include <queue>

using namespace std;

namespace Logger {
	thread m_logger_thread;
	mutex *m_lock = nullptr;
	queue<string> *m_queue = nullptr;
	ofstream m_file;
	chrono::seconds m_reopen_interval = std::chrono::seconds(300);
	chrono::system_clock::time_point m_last_reopen;
	bool m_verbose = true;
	bool m_run_logger = true;
	bool m_logger_started = false;

	void verbose(bool verbose) {
		m_verbose = verbose;
	}

	void initialize() {
		m_lock = new mutex;
		m_queue = new queue<string>;
	}

	void de_initialize() {
		delete m_lock;
		delete m_queue;
	}

	void reopen() {
		auto now = chrono::system_clock::now();
		m_lock->lock();
		if (now - m_last_reopen > m_reopen_interval) {
			m_last_reopen = now;
			try {
				m_file.close();
			} catch (...) {

			}
			try {
				m_file.open(Config::log_file_path, ofstream::out | ofstream::app);
				m_last_reopen = chrono::system_clock::now();
			} catch (exception &error) {
				try {
					m_file.close();
				} catch (...) {
					
				}
				throw error;
			}
		}
		m_lock->unlock();
	}

	string timestamp() {
		chrono::system_clock::time_point tp = std::chrono::system_clock::now();
		time_t tt = std::chrono::system_clock::to_time_t(tp);
		tm gmt{}; gmtime_r(&tt, &gmt);
		string buffer(100, 'x');
		sprintf(&buffer.front(), "%04d-%02d-%02d %02d:%02d:%02d", gmt.tm_year + 1900, (short)gmt.tm_mon + 1,
			(short)gmt.tm_mday, (short)gmt.tm_hour, (short)gmt.tm_min, (short)gmt.tm_sec);
		buffer.resize(19);
		return buffer;
	}

	string format(const string &type, const string &file, int line, const string &message, const string &meta) {
		string output;
		output.append(timestamp());
		output.append(" [" + type + "]");
		output.append(" " + file + ":" + to_string(line));
		output.append(" " + message);
		output.append(" " + meta);
		return output;
	}

	void log_message(const string &type, const string &file, int line, const string &message, const string &meta) {
		log_string(format(type, file, line, message, meta));
	}

	void log_string(const string &message) {
		if (m_lock == nullptr) return; // logger thread not started.
		m_lock->lock();
		m_queue->push(message);
		m_lock->unlock();
	}

	void log(const string &type, const string &file, int line, const string &message) {
		log_message(type, file, line, message, "");
	}

	void write_message_to_logfile(const string &message) {
		m_file << message << endl;
	}

	void logger_thread() {
		initialize();
		reopen();
		while (true) {
			while (m_queue->empty() && m_run_logger) {
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}

			if (m_queue->empty()) break;

			m_lock->lock();
			string message = m_queue->front();
			m_queue->pop();
			m_lock->unlock();

			write_message_to_logfile(message);
		}

		de_initialize();
	}

	void start_logger_thread() {
		if (!m_logger_started) {
			m_logger_thread = thread(logger_thread);
			m_logger_started = true;
		}
	}

	void join_logger_thread() {
		if (m_logger_started) {
			m_run_logger = false;
			m_logger_thread.join();
			m_logger_started = false;
		}
	}

	LoggedException::LoggedException(const string &message, const string &file, int line)
	: m_message(message), m_file(file), m_line(line)
	{
		m_formatted_message = format("EXCEPTION", m_file, m_line, m_message, "");
	}
}

