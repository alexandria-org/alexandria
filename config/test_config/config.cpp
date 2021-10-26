
#include "config.h"

namespace Config {

	unsigned long long nodes_in_cluster = 1;
	unsigned long long node_id = 0;
	unsigned long long ft_max_results_per_section = 100000;

	std::vector<std::string> batches = {
		"ALEXANDRIA-MANUAL-01",
		"CC-MAIN-2021-31"
	};

	std::vector<std::string> link_batches = {
		"CC-MAIN-2021-31"
	};

}

