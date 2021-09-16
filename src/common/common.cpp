
#include "common.h"

#include <sstream>
#include <iomanip>

std::string escape_json(const std::string &s) {
	std::ostringstream o;
	for (auto c = s.cbegin(); c != s.cend(); c++) {
		if (*c == '"' || *c == '\\' || ('\x00' <= *c && *c <= '\x1f')) {
			o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)*c;
		} else {
			o << *c;
		}
	}
	return o.str();
}

std::function<std::shared_ptr<Aws::Utils::Logging::LogSystemInterface>()> get_logger_factory() {
	return [] {
		return Aws::MakeShared<Aws::Utils::Logging::ConsoleLogSystem>(
			"console_logger", Aws::Utils::Logging::LogLevel::Error);
	};
}
