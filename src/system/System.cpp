
#include "System.h"

namespace System {

	bool is_dev() {
		if (getenv("ALEXANDRIA_LIVE") != NULL && stoi(getenv("ALEXANDRIA_LIVE")) > 0) {
			return false;
		}
		return true;
	}

	string domain_index_filename() {
		if (is_dev()) {
			return "/dev_files/domain_info.tsv";
		}
		return "/files/domain_info.tsv";
	}

	string dictionary_filename() {
		if (is_dev()) {
			return "/dev_files/dictionary.tsv";
		}
		return "/files/dictionary.tsv";
	}

}
