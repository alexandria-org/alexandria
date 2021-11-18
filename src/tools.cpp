
#include "config.h"
#include "tools/Splitter.h"
#include "tools/CalculateHarmonic.h"
#include <iostream>

using namespace std;

void help() {
	cout << "Usage: ./tools [OPTION]..." << endl;
	cout << "--split run splitter" << endl;
}

int main(int argc, const char **argv) {

	if (getenv("ALEXANDRIA_CONFIG") != NULL) {
		Config::read_config(getenv("ALEXANDRIA_CONFIG"));
	} else {
		Config::read_config("/etc/alexandria.conf");
	}

	if (argc < 2) {
		help();
		return 0;
	}

	const string arg(argv[1]);

	if (arg == "--split") {
		Tools::run_splitter();
	} else if (arg == "--harmonic-hosts") {
		Tools::calculate_harmonic_hosts();
	} else if (arg == "--harmonic-links") {
		Tools::calculate_harmonic_links();
	} else if (arg == "--harmonic") {
		Tools::calculate_harmonic();
	} else {
		help();
	}

	return 0;
}
