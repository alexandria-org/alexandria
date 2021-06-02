

#pragma once

#include <mutex>
#include <fstream>
#include <iostream>

#define LogInfo(msg) Logger::log("info", __FILE__, __LINE__, msg)
#define LogError(msg) Logger::log("error", __FILE__, __LINE__, msg)

#define error(msg) runtime_error(string(__FILE__) + ":" + to_string(__LINE__) + " " + msg)

class LoggedException

using namespace std;

class Logger {

public:

	static string m_filename;

	Logger();
	~Logger();

	void verbose(bool verbose);
	void reopen();
	string timestamp();
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

