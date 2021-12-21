
#include "Worker.h"
#include "fcgio.h"
#include "config.h"
#include "parser/URL.h"
#include "parser/cc_parser.h"

#include "post_processor/PostProcessor.h"
#include "api/ApiResponse.h"
#include "hash_table/HashTable.h"
#include "full_text/FullText.h"
#include "full_text/FullTextIndex.h"
#include "full_text/FullTextRecord.h"
#include "full_text/SearchMetric.h"
#include "search_engine/SearchAllocation.h"
#include "Api.h"
#include "ApiStatusResponse.h"
#include "link/FullTextRecord.h"
#include "system/Logger.h"

using namespace std;

namespace Worker {

	void test_search(const string &query) {
		SearchAllocation::Allocation *allocation = SearchAllocation::create_allocation();

		HashTable hash_table("main_index");
		HashTable hash_table_link("link_index");
		HashTable hash_table_domain_link("domain_link_index");

		vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("main_index");
		vector<FullTextIndex<Link::FullTextRecord> *> link_index_array = FullText::create_index_array<Link::FullTextRecord>("link_index");
		vector<FullTextIndex<DomainLink::FullTextRecord> *> domain_link_index_array =
			FullText::create_index_array<DomainLink::FullTextRecord>("domain_link_index");

		stringstream response_stream;
		Api::search(query, hash_table, index_array, link_index_array, domain_link_index_array, allocation, response_stream);

		cout << response_stream.rdbuf() << endl;

		FullText::delete_index_array<FullTextRecord>(index_array);
		FullText::delete_index_array<Link::FullTextRecord>(link_index_array);
		FullText::delete_index_array<DomainLink::FullTextRecord>(domain_link_index_array);

		SearchAllocation::delete_allocation(allocation);
	}

	void output_response(FCGX_Request &request, stringstream &response) {

		FCGX_FPrintF(request.out, "Content-type: application/json\r\n\r\n");
		FCGX_FPrintF(request.out, "%s", response.str().c_str());

	}

	void *run_worker(void *data) {

		SearchAllocation::Allocation *allocation = SearchAllocation::create_allocation();

		Worker *worker = static_cast<Worker *>(data);

		FCGX_Request request;

		FCGX_InitRequest(&request, worker->socket_id, 0);

		HashTable hash_table("main_index");
		HashTable hash_table_link("link_index");
		HashTable hash_table_domain_link("domain_link_index");

		vector<FullTextIndex<FullTextRecord> *> index_array = FullText::create_index_array<FullTextRecord>("main_index");
		vector<FullTextIndex<Link::FullTextRecord> *> link_index_array =
			FullText::create_index_array<Link::FullTextRecord>("link_index");
		vector<FullTextIndex<DomainLink::FullTextRecord> *> domain_link_index_array =
			FullText::create_index_array<DomainLink::FullTextRecord>("domain_link_index");

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
		FullText::delete_index_array<Link::FullTextRecord>(link_index_array);
		FullText::delete_index_array<DomainLink::FullTextRecord>(domain_link_index_array);

		SearchAllocation::delete_allocation(allocation);

		return NULL;
	}

	void start_server() {
		FCGX_Init();

		int socket_id = FCGX_OpenSocket("127.0.0.1:8000", 20);
		if (socket_id < 0) {
			LOG_INFO("Could not open socket, exiting");
			return;
		}

		vector<pthread_t> thread_ids(Config::worker_count);

		Worker *workers = new Worker[Config::worker_count];
		for (size_t i = 0; i < Config::worker_count; i++) {
			workers[i].socket_id = socket_id;
			workers[i].thread_id = i;

			pthread_create(&thread_ids[i], NULL, run_worker, &workers[i]);
		}

		for (size_t i = 0; i < Config::worker_count; i++) {
			pthread_join(thread_ids[i], NULL);
		}
	}

	void download_server() {
		Parser::warc_downloader();
	}

	void status_server(Status *status) {
		FCGX_Init();

		int socket_id = FCGX_OpenSocket("127.0.0.1:8000", 20);
		if (socket_id < 0) {
			LOG_INFO("Could not open socket, exiting");
			return;
		}

		FCGX_Request request;
		FCGX_InitRequest(&request, socket_id, 0);

		LOG_INFO("Status server has started...");

		while (true) {

			int accept_response = FCGX_Accept_r(&request);
			if (accept_response < 0) {
				break;
			}

			stringstream response_stream;

			ApiStatusResponse api_response(*status);

			response_stream << api_response;

			output_response(request, response_stream);

			FCGX_Finish_r(&request);
		}
	}

	thread download_server_thread;
	void start_download_server() {

		download_server_thread = std::move(thread(download_server));

	}

	thread status_server_thread;
	void start_status_server(Status &status) {

		status_server_thread = std::move(thread(status_server, &status));

	}
}
