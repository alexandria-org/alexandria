
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

namespace Profiler {

	class instance {

	public:

		instance(const string &name);
		instance();
		~instance();

		void enable();
		double get() const;
		double get_micro() const;
		void stop();
		void print();

	private:
		string m_name;
		bool m_enabled = true;
		bool m_has_stopped = false;
		std::chrono::_V2::system_clock::time_point m_start_time;
	};

	void print_memory_status();
	uint64_t get_cycles();

}
