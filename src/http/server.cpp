/*
 * MIT License
 *
 * Alexandria.org
 *
 * Copyright (c) 2021 Josef Cullhed, <info@alexandria.org>, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "server.h"
#include "fcgio.h"
#include "logger/logger.h"
#include "URL.h"

#include <thread>
#include <vector>

namespace http {

	server::server(std::function<http::response(const http::request &)> handler) {
		m_handler = handler;

		start();
	}

	void server::run_worker(int socket_id) {

		FCGX_Request request;

		FCGX_InitRequest(&request, socket_id, 0);

		LOG_INFO("Server has started...");

		while (true) {

			m_lock.lock();
			int accept_response = FCGX_Accept_r(&request);
			m_lock.unlock();

			if (accept_response < 0) {
				break;
			}

			const char *uri_ptr = FCGX_GetParam("REQUEST_URI", request.envp);
			const char *req_ptr = FCGX_GetParam("REQUEST_METHOD", request.envp);
			if ((uri_ptr == nullptr) || (req_ptr == nullptr)) {
				FCGX_Finish_r(&request);
				continue;
			}
			std::string uri(uri_ptr);
			std::string request_method(req_ptr);

			LOG_INFO("Serving request: " + uri);

			URL url("http://alexandria.org" + uri);

			::http::request http_request(url);

			::http::response http_response = m_handler(http_request);

			const std::string data_out = http_response.body();

			// Output response
			const std::string content_type = std::string("Content-type: ") + http_response.content_type() + "\r\n";
			const std::string status = std::string("Status: ") + std::to_string(http_response.code()) + "\r\n";
			const std::string end_req = "\r\n";

			FCGX_FPrintF(request.out, status.c_str());
			FCGX_FPrintF(request.out, content_type.c_str());
			FCGX_FPrintF(request.out, end_req.c_str());
			FCGX_PutStr(data_out.c_str(), data_out.size(), request.out);

			FCGX_Finish_r(&request);
		}

		FCGX_Free(&request, true);
	}

	void server::start() {
		FCGX_Init();

		int socket_id = FCGX_OpenSocket("127.0.0.1:8000", 20);
		if (socket_id < 0) {
			LOG_INFO("Could not open socket, exiting");
			return;
		}

		std::vector<std::thread> threads;

		for (size_t i = 0; i < m_workers; i++) {
			threads.emplace_back(std::move(std::thread([this](int socket_id){ run_worker(socket_id); }, socket_id)));
		}

		for (auto &thread : threads) {
			thread.join();
		}

		close(socket_id);
	}

}
