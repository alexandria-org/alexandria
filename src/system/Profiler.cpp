
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

void Profiler::enable() {
	m_enabled = true;
}

double Profiler::get() const {
	if (!m_enabled) return 0;
	auto timer_elapsed = chrono::high_resolution_clock::now() - m_start_time;
	auto microseconds = chrono::duration_cast<std::chrono::microseconds>(timer_elapsed).count();

	return (double)microseconds/1000;
}

double Profiler::get_micro() const {
	if (!m_enabled) return 0;
	auto timer_elapsed = chrono::high_resolution_clock::now() - m_start_time;
	auto microseconds = chrono::duration_cast<std::chrono::microseconds>(timer_elapsed).count();

	return (double)microseconds;
}

void Profiler::stop() {
	if (!m_enabled) return;
	m_has_stopped = true;
	cout << "Profiler [" << m_name << "] took " << get() << "ms" << endl;
}

void Profiler::print() {
	if (!m_enabled) return;
	cout << "Profiler [" << m_name << "] took " << get() << "ms" << endl;
}

void Profiler::print_memory_status() {
	ifstream infile("/proc/" + to_string(getpid()) + "/status");
	if (infile.is_open()) {
		string line;
		while (getline(infile, line)) {
			cout << line << endl;
		}
	}
}

