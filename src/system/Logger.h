

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

