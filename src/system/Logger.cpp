
#include "Logger.h"

string Logger::m_filename = "/var/log/alexandria.log";

Logger *Logger::instance() {
	static Logger instance;
	return &instance;
}

Logger::Logger() {
	m_verbose = false;
	m_reopen_interval = std::chrono::seconds(300);

	reopen();
}

Logger::~Logger() {
	m_file.close();
}

void Logger::verbose(bool verbose) {
	m_verbose = verbose;
}

void Logger::reopen() {
	auto now = chrono::system_clock::now();
	m_lock.lock();
	if (now - m_last_reopen > m_reopen_interval) {
		m_last_reopen = now;
		try {
			m_file.close();
		} catch (...) {

		}
		try {
			m_file.open(m_filename, ofstream::out | ofstream::app);
			m_last_reopen = chrono::system_clock::now();
		} catch (exception &error) {
			try {
				m_file.close();
			} catch (...) {
				
			}
			throw error;
		}
	}
	m_lock.unlock();
}

string Logger::timestamp() {
	chrono::system_clock::time_point tp = std::chrono::system_clock::now();
	time_t tt = std::chrono::system_clock::to_time_t(tp);
	tm gmt{}; gmtime_r(&tt, &gmt);
	string buffer(100, 'x');
	sprintf(&buffer.front(), "%04d-%02d-%02d %02d:%02d:%02d", gmt.tm_year + 1900, (short)gmt.tm_mon + 1,
		(short)gmt.tm_mday, (short)gmt.tm_hour, (short)gmt.tm_min, (short)gmt.tm_sec);
	buffer.resize(19);
	return buffer;
}

void Logger::log_message(const string &type, const string &file, int line, const string &message, const string &meta) {
	string output;
	output.append(timestamp());
	output.append(" [" + type + "]");
	output.append(" " + file + ":" + to_string(line));
	output.append(" " + message);
	output.append(" " + meta);
	log_string(output);
}

void Logger::log_string(const string &message) {
	if (m_verbose) {
		cerr << message << endl;
	}
	m_lock.lock();
	m_file << message << endl;
	m_file.flush();
	m_lock.unlock();
	reopen();
}

void Logger::log(const string &type, const string &file, int line, const string &message) {
	Logger::instance()->log_message(type, file, line, message, "");
}

void Logger::log(const string &type, const string &file, int line, const string &message, const string &meta) {
	Logger::instance()->log_message(type, file, line, message, meta);
}

