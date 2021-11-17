
#include "config.h"
#include "tools/Splitter.h"
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
		Config::read_config("config.conf");
	}

	if (argc < 2) {
		help();
		return 0;
	}

	const string arg(argv[1]);

	if (arg == "--split") {
		Tools::run_splitter();
	} else {
		help();
	}

	return 0;
}
