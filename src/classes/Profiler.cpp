
#include "Profiler.h"

Profiler::Profiler(const string &name) :
	m_name(name)
{
	m_start_time = std::chrono::high_resolution_clock::now();
}

Profiler::~Profiler() {
	if (!m_has_stopped) {
		stop();
	}
}

void Profiler::stop() {
	m_has_stopped = true;
	auto timer_elapsed = chrono::high_resolution_clock::now() - m_start_time;
	auto microseconds = chrono::duration_cast<std::chrono::microseconds>(timer_elapsed).count();
	cout << "Profiler [" << m_name << "] took " << (microseconds/1000) << "ms" << endl;
}

void Profiler::print() {
	auto timer_elapsed = chrono::high_resolution_clock::now() - m_start_time;
	auto microseconds = chrono::duration_cast<std::chrono::microseconds>(timer_elapsed).count();
	cout << "Profiler [" << m_name << "] took " << (microseconds/1000) << "ms" << endl;
}