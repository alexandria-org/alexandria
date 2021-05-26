
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

double Profiler::get() const {
	auto timer_elapsed = chrono::high_resolution_clock::now() - m_start_time;
	auto microseconds = chrono::duration_cast<std::chrono::microseconds>(timer_elapsed).count();

	return (double)microseconds/1000;
}

void Profiler::stop() {
	m_has_stopped = true;
	cout << "Profiler [" << m_name << "] took " << get() << "ms" << endl;
}

void Profiler::print() {
	cout << "Profiler [" << m_name << "] took " << get() << "ms" << endl;
}
