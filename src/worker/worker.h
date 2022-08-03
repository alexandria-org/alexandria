
#pragma once

#include <iostream>

namespace worker {

	struct status {

		size_t items;
		size_t items_indexed;
		size_t start_time;

	};

	struct worker_data {

		int socket_id;
		int thread_id;

	};

	void test_search(const std::string &query);
	void start_server();
	void start_download_server();
	void start_status_server(status &status);
	void start_scraper_server();
	void wait_for_scraper_server();

}

