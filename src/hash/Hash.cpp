
#include "Hash.h"

namespace Hash {

	hash<string> hasher;

	size_t str(const string &str) {
		return hasher(str);
	}

}
