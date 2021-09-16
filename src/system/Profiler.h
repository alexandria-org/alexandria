
#pragma once

#include <iostream>
#include <chrono>
#include <fstream>
#include <unistd.h>

#if __has_include("x86intrin.h")

	#define PROFILE_CPU_CYCLES true
	#include <x86intrin.h>

#else
	#define PROFILE_CPU_CYCLES false
#endif

using namespace std;

class Profiler {

public:

	Profiler(const string &name);
	~Profiler();

	void enable();
	double get() const;
	double get_micro() const;
	void stop();
	void print();
	uint64_t get_cycles() const;

	static void print_memory_status();

private:
	bool m_enabled = true;
	bool m_has_stopped = false;
	string m_name;
	std::chrono::_V2::system_clock::time_point m_start_time;
};
