
#include "worker.h"
#include "fcgio.h"
#include "config.h"
#include "parser/URL.h"
#include "parser/cc_parser.h"
#include <pthread.h>
#include <signal.h>
#include <boost/filesystem.hpp>

#include "post_processor/PostProcessor.h"
#include "hash_table/HashTable.h"
#include "full_text/FullText.h"
#include "full_text/FullTextIndex.h"
#include "full_text/FullTextRecord.h"
#include "full_text/SearchMetric.h"
#include "search_engine/SearchAllocation.h"
#include "api/api.h"
#include "api/api_status_response.h"
#include "link/FullTextRecord.h"
#include "logger/logger.h"
#include "system/Profiler.h"
#include "urlstore/UrlStore.h"
#include "scraper/scraper.h"

using namespace std;
using namespace std::literals::chrono_literals;

namespace worker {

	void test_search(const string &query) {
		SearchAllocation::Allocation *allocation = SearchAllocation::create_allocation();

		HashTable hash_table("main_index");
		HashTable hash_table_link("link_index");
		HashTable hash_table_domain_link("domain_link_index");

		FullTextIndex<FullTextRecord> index("main_index");
		FullTextIndex<Link::FullTextRecord> link_index("link_index");
		FullTextIndex<DomainLink::FullTextRecord> domain_link_index("domain_link_index");

		stringstream response_stream;
		api::search(query, hash_table, index, link_index, domain_link_index, allocation, response_stream);

		cout << response_stream.rdbuf() << endl;

		SearchAllocation::delete_allocation(allocation);
	}

	void output_response(FCGX_Request &request, stringstream &response) {

		FCGX_FPrintF(request.out, "Content-type: application/json\r\n\r\n");
		FCGX_FPrintF(request.out, "%s", response.str().c_str());

	}

	void output_binary_response(FCGX_Request &request, stringstream &response) {

		FCGX_FPrintF(request.out, "Content-type: application/octet-stream\r\n\r\n");
		string data_out = response.str();
		FCGX_PutStr(data_out.c_str(), response.str().size(), request.out);

	}

	void *run_worker(void *data) {

		SearchAllocation::Allocation *allocation = SearchAllocation::create_allocation();

		worker_data *worker = static_cast<worker_data *>(data);

		FCGX_Request request;

		FCGX_InitRequest(&request, worker->socket_id, 0);

		HashTable hash_table("main_index");
		HashTable hash_table_link("link_index");
		HashTable hash_table_domain_link("domain_link_index");

		FullTextIndex<FullTextRecord> index("main_index");
		FullTextIndex<Link::FullTextRecord> link_index("link_index");
		FullTextIndex<DomainLink::FullTextRecord> domain_link_index("domain_link_index");

		LOG_INFO("Server has started...");

		while (true) {

			static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;

			pthread_mutex_lock(&accept_mutex);
			int accept_response = FCGX_Accept_r(&request);
			pthread_mutex_unlock(&accept_mutex);

			if (accept_response < 0) {
				break;
			}

			const char *uri_ptr = FCGX_GetParam("REQUEST_URI", request.envp);
			const char *req_ptr = FCGX_GetParam("REQUEST_METHOD", request.envp);
			if ((uri_ptr == nullptr) || (req_ptr == nullptr)) {
				FCGX_Finish_r(&request);
				continue;
			}
			string uri(uri_ptr);
			string request_method(req_ptr);

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
				if (Config::index_text) {
					api::search(query["q"], hash_table, index, link_index, domain_link_index, allocation, response_stream);
					output_response(request, response_stream);
				} else {
					api::search_remote(query["q"], hash_table, link_index, domain_link_index, allocation, response_stream);
					output_response(request, response_stream);
				}
			} else if (query.find("q") != query.end() && !deduplicate) {
				api::search_all(query["q"], hash_table, index, link_index, domain_link_index, allocation, response_stream);
				output_response(request, response_stream);
			} else if (query.find("s") != query.end()) {
				api::word_stats(query["s"], index, link_index, hash_table.size(), hash_table_link.size(), response_stream);
				output_response(request, response_stream);
			} else if (query.find("u") != query.end()) {
				api::url(query["u"], hash_table, response_stream);
				output_response(request, response_stream);
			} else if (query.find("i") != query.end()) {
				api::ids(query["i"], index, allocation, response_stream);
				output_binary_response(request, response_stream);
			}

			FCGX_Finish_r(&request);
		}

		SearchAllocation::delete_allocation(allocation);

		FCGX_Free(&request, true);

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

		worker_data *workers = new worker_data[Config::worker_count];
		for (size_t i = 0; i < Config::worker_count; i++) {
			workers[i].socket_id = socket_id;
			workers[i].thread_id = i;

			pthread_create(&thread_ids[i], NULL, run_worker, &workers[i]);
		}

		for (size_t i = 0; i < Config::worker_count; i++) {
			pthread_join(thread_ids[i], NULL);
		}

		close(socket_id);
	}

	void download_server() {
		Parser::warc_downloader();
	}

	void status_server(status *status) {
		FCGX_Init();

		int socket_id = FCGX_OpenSocket("127.0.0.1:8000", 20);
		if (socket_id < 0) {
			LOG_INFO("Could not open socket, exiting");
			return;
		}

		FCGX_Request request;
		FCGX_InitRequest(&request, socket_id, 0);

		LOG_INFO("status server has started...");

		while (true) {

			int accept_response = FCGX_Accept_r(&request);
			if (accept_response < 0) {
				break;
			}

			stringstream response_stream;

			api::api_status_response api_response(*status);

			response_stream << api_response;

			output_response(request, response_stream);

			FCGX_Finish_r(&request);
		}

		FCGX_Free(&request, true);
		close(socket_id);
	}

	void urlstore_worker(UrlStore::all_stores &stores, int socket_id, mutex &accept_mutex) {

		FCGX_Request request;
		FCGX_InitRequest(&request, socket_id, 0);

		LOG_INFO("Urlstore worker has started...");

		const size_t max_post_len = 1024*1024*1024;
		const size_t buffer_len = 1024*1024;
		char *buffer = new char[buffer_len];

		int error = 0;
		while (true) {

			error = 0;

			accept_mutex.lock();
			int accept_response = FCGX_Accept_r(&request);
			accept_mutex.unlock();
			if (accept_response < 0) {
				break;
			}

			const char *uri_ptr = FCGX_GetParam("REQUEST_URI", request.envp);
			const char *req_ptr = FCGX_GetParam("REQUEST_METHOD", request.envp);

			if ((uri_ptr == nullptr) || (req_ptr == nullptr)) {
				FCGX_Finish_r(&request);
				continue;
			}

			string uri(uri_ptr);
			string request_method(req_ptr);

			// /store/[realm]/
			uri.replace(0, 7, "");
			const size_t slash_pos = uri.find("/");
			const string type = uri.substr(0, slash_pos);
			const string public_key = uri.substr(slash_pos + 1);

			stringstream response_stream;

			if (request_method == "PUT") {
				string post_data;
				while (true) {

					const size_t read_bytes = FCGX_GetStr(buffer, buffer_len, request.in);
					if (read_bytes == 0) break;

					if (post_data.size() + read_bytes > max_post_len) {
						error = 1;
						LOG_ERROR("Posted data larger then " + to_string(max_post_len) + ", ignoring request");
						break;
					}
					post_data.append(buffer, read_bytes);
				}

				if (error == 0) {
					if (type == "url") {
						UrlStore::handle_put_request(stores.url, post_data, response_stream);
					}
					if (type == "domain") {
						UrlStore::handle_put_request(stores.domain, post_data, response_stream);
					}
					if (type == "robots") {
						UrlStore::handle_put_request(stores.robots, post_data, response_stream);
					}
					FCGX_FPrintF(request.out, "Content-type: text/html\r\n\r\n");
					FCGX_FPrintF(request.out, "%s", "Data Uploaded");
				}
			} else if (request_method == "GET") {

				//char **env = request.envp;
				//while (*(++env)) cout << *env << endl;

				const char *accept_ptr = FCGX_GetParam("HTTP_ACCEPT", request.envp);
				string accept = "text/html";
				if (accept_ptr != nullptr) {
					accept = string(accept_ptr);
				}
				
				if (accept == "application/octet-stream") {
					if (type == "url") {
						UrlStore::handle_binary_get_request(stores.url, public_key, response_stream);
					}
					if (type == "domain") {
						UrlStore::handle_binary_get_request(stores.domain, public_key, response_stream);
					}
					if (type == "robots") {
						UrlStore::handle_binary_get_request(stores.robots, public_key, response_stream);
					}
					output_binary_response(request, response_stream);
				} else {
					if (type == "url") {
						UrlStore::handle_get_request(stores.url, public_key, response_stream);
					}
					if (type == "domain") {
						UrlStore::handle_get_request(stores.domain, public_key, response_stream);
					}
					if (type == "robots") {
						UrlStore::handle_get_request(stores.robots, public_key, response_stream);
					}
					output_response(request, response_stream);
				}
			} else if (request_method == "POST") {

				const char *accept_ptr = FCGX_GetParam("HTTP_ACCEPT", request.envp);
				string accept = "text/html";
				if (accept_ptr != nullptr) {
					accept = string(accept_ptr);
				}

				string post_data;
				Profiler::instance prof("[urlstore] read post data");
				while (true) {

					const size_t read_bytes = FCGX_GetStr(buffer, buffer_len, request.in);
					if (read_bytes == 0) break;

					if (post_data.size() + read_bytes > max_post_len) {
						error = 1;
						LOG_ERROR("Posted data larger then " + to_string(max_post_len) + ", ignoring request");
						break;
					}
					post_data.append(buffer, read_bytes);
				}
				prof.stop();

				if (error == 0) {
					if (accept == "application/octet-stream") {
						if (type == "url") {
							UrlStore::handle_binary_post_request(stores.url, post_data, response_stream);
						}
						if (type == "domain") {
							UrlStore::handle_binary_post_request(stores.domain, post_data, response_stream);
						}
						if (type == "robots") {
							UrlStore::handle_binary_post_request(stores.robots, post_data, response_stream);
						}
						output_binary_response(request, response_stream);
					} else {
						if (type == "url") {
							UrlStore::handle_post_request(stores.url, post_data, response_stream);
						}
						if (type == "domain") {
							UrlStore::handle_post_request(stores.domain, post_data, response_stream);
						}
						if (type == "robots") {
							UrlStore::handle_post_request(stores.robots, post_data, response_stream);
						}
						output_response(request, response_stream);
					}
				}
			}


			FCGX_Finish_r(&request);
		}

		FCGX_Free(&request, true);
	}

	thread download_server_thread;
	void start_download_server() {

		download_server_thread = std::move(thread(download_server));

	}

	thread status_server_thread;
	void start_status_server(status &status) {

		status_server_thread = std::move(thread(status_server, &status));

	}

	void start_urlstore_workers() {

		boost::filesystem::create_directories(Config::url_store_cache_path);

		UrlStore::all_stores stores;

		thread url_inserter_thread(UrlStore::urlstore_inserter<UrlStore::UrlData>, std::ref(stores.url));
		thread domain_inserter_thread(UrlStore::urlstore_inserter<UrlStore::DomainData>, std::ref(stores.domain));
		thread robots_inserter_thread(UrlStore::urlstore_inserter<UrlStore::RobotsData>, std::ref(stores.robots));

		FCGX_Init();
		int socket_id = FCGX_OpenSocket("127.0.0.1:8001", 20);
		if (socket_id < 0) {
			LOG_INFO("Could not open socket, exiting");
			return;
		}

		vector<thread> threads;

		mutex accept_mutex;

		for (size_t i = 0; i < Config::worker_count; i++) {
			threads.emplace_back(thread(urlstore_worker, ref(stores), socket_id, ref(accept_mutex)));
		}

		for (auto &thread : threads) {
			thread.join();
		}

		url_inserter_thread.join();
		domain_inserter_thread.join();
		robots_inserter_thread.join();

		close(socket_id);
	}

	thread urlstore_server_thread;
	void start_urlstore_server() {

		urlstore_server_thread = std::move(thread(start_urlstore_workers));
		this_thread::sleep_for(500ms); // Wait for the threads to start...
	}

	void wait_for_urlstore_server() {

		urlstore_server_thread.join();

	}

	void scraper_server() {
		Scraper::url_downloader();
	}

	thread scraper_server_thread;
	void start_scraper_server() {

		scraper_server_thread = std::move(thread(scraper_server));

	}

	void wait_for_scraper_server() {
		scraper_server_thread.join();
	}

}
