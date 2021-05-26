
#pragma once

#include <iostream>
#include <chrono>

using namespace std;

class Profiler {

public:

	Profiler(const string &name);
	~Profiler();

	double get() const;
	void stop();
	void print();

private:
	bool m_has_stopped = false;
	string m_name;
	std::chrono::_V2::system_clock::time_point m_start_time;
};