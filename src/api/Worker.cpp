
#include "Worker.h"

namespace Worker {

	void output_response(FCGX_Request &request, stringstream &response) {

		FCGX_FPrintF(request.out, "Content-type: application/json\r\n\r\n");
		FCGX_FPrintF(request.out, "%s", response.str().c_str());

	}

	void *run_worker(void *data) {

		SearchAllocation::Allocation *allocation = SearchAllocation::create_allocation();

		Worker *worker = (Worker *)data;
		int rc, i, thread_id = worker->thread_id;
		pid_t pid = getpid();

		FCGX_Request request;

		FCGX_InitRequest(&request, worker->socket_id, 0);

		HashTable hash_table("main_index");
		HashTable hash_table_link("link_index");
		HashTable hash_table_domain_link("domain_link_index");

		vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("main_index");
		vector<FullTextIndex<LinkFullTextRecord> *> link_index_array =
			FullText::create_index_array<LinkFullTextRecord>("link_index");
		vector<FullTextIndex<DomainLinkFullTextRecord> *> domain_link_index_array =
			FullText::create_index_array<DomainLinkFullTextRecord>("domain_link_index");

		LOG_INFO("Server has started...");

		while (true) {

			static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;

			pthread_mutex_lock(&accept_mutex);
			int accept_response = FCGX_Accept_r(&request);
			pthread_mutex_unlock(&accept_mutex);

			if (accept_response < 0) {
				break;
			}

			string uri = FCGX_GetParam("REQUEST_URI", request.envp);

			LOG_INFO("Serving request: " + uri);

			URL url("http://alexandria.org" + uri);

			auto query = url.query();

			stringstream response_stream;

			bool deduplicate = true;
			if (query.find("d") != query.end()) {
				if (query["d"] == "a") {
					deduplicate = false;
				}
			}

			if (query.find("q") != query.end() && deduplicate) {
				Api::search(query["q"], hash_table, index_array, link_index_array, domain_link_index_array, allocation, response_stream);
			} else if (query.find("q") != query.end() && !deduplicate) {
				Api::search_all(query["q"], hash_table, index_array, link_index_array, domain_link_index_array, allocation, response_stream);
			} else if (query.find("s") != query.end()) {
				Api::word_stats(query["s"], index_array, link_index_array, hash_table.size(), hash_table_link.size(), response_stream);
			} else if (query.find("u") != query.end()) {
				Api::url(query["u"], hash_table, response_stream);
			}

			output_response(request, response_stream);

			FCGX_Finish_r(&request);
		}

		FullText::delete_index_array<FullTextRecord>(index_array);
		FullText::delete_index_array<LinkFullTextRecord>(link_index_array);
		FullText::delete_index_array<DomainLinkFullTextRecord>(domain_link_index_array);

		SearchAllocation::delete_allocation(allocation);

		return NULL;
	}
}
