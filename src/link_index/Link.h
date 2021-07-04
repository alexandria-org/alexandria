
#pragma once

#include "parser/URL.h"

struct Link {
	URL source_url;
	URL target_url;
	uint64_t target_host_hash;
	int source_harmonic;
	int target_harmonic;
};

