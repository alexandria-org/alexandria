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

#include "config.h"
#include "transfer.h"
#include <fstream>
#include "common/ThreadPool.h"
#include "logger/logger.h"
#include "profiler/profiler.h"
#include "file/file.h"
#include "text/text.h"
#include "parser/parser.h"
#include "algorithm/hash.h"

using namespace std;

namespace transfer {

	size_t curl_stringstream_writer(void *ptr, size_t size, size_t nmemb, stringstream *ss) {
		size_t byte_size = size * nmemb;
		ss->write((char *)ptr, byte_size);
		return byte_size;
	}

	size_t curl_ostream_writer(void *ptr, size_t size, size_t nmemb, ostream *os) {
		size_t byte_size = size * nmemb;
		os->write((char *)ptr, byte_size);
		return byte_size;
	}

	size_t curl_string_writer(void *ptr, size_t size, size_t nmemb, string *str) {
		size_t byte_size = size * nmemb;
		str->append((char *)ptr, byte_size);
		return byte_size;
	}

	struct curl_string_read_struct {
		const char *buffer;
		size_t buffer_len;
		size_t offset;
	};

	size_t curl_string_reader(char *ptr, size_t size, size_t nmemb, void *userdata) {
		struct curl_string_read_struct *arg = (struct curl_string_read_struct *)userdata;

		if (arg->offset >= arg->buffer_len) {
			return 0ull;
		}

		size_t max_read = size * nmemb;
		size_t read_bytes = arg->buffer_len - arg->offset;
		if (read_bytes > max_read) read_bytes = max_read;

		memcpy(ptr, &arg->buffer[arg->offset], read_bytes);

		arg->offset += read_bytes;

		return read_bytes;
	}

	size_t curl_file_reader(char *ptr, size_t size, size_t nmemb, void *userdata) {
		std::ifstream *infile = (std::ifstream *)userdata;

		if (infile->eof()) {
			return 0ull;
		}

		size_t max_read = size * nmemb;

		infile->read(ptr, max_read);

		return infile->gcount();
	}

	void set_internal_auth(CURL *curl) {
		curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());
		curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
	}

	string make_url(const string &url) {

		if (url.find("http://") == 0 || url.find("https://") == 0) {
			return url;
		}

		if (url.size() && url[0] != '/') {
			return "http://" + config::master + "/" + url;
		}
		return "http://" + config::master + url;
	}

	string file_to_string(const string &file_path, int &error) {
		CURL *curl = curl_easy_init();
		error = ERROR;
		if (curl) {
			CURLcode res;
			LOG_INFO("Downloading url: " + make_url(file_path));
			curl_easy_setopt(curl, CURLOPT_URL, make_url(file_path).c_str());

			set_internal_auth(curl);

			stringstream response;
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_stringstream_writer);

			res = curl_easy_perform(curl);

			if (res == CURLE_OK) {
				long response_code;
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
				if (response_code == 200) {
					error = OK;
				}
			}

			curl_easy_cleanup(curl);

			return response.str();
		}

		return "";
	}

	string gz_file_to_string(const string &file_path, int &error) {
		CURL *curl = curl_easy_init();
		error = ERROR;
		if (curl) {
			CURLcode res;
			LOG_INFO("Downloading url: " + make_url(file_path));
			curl_easy_setopt(curl, CURLOPT_URL, make_url(file_path).c_str());

			set_internal_auth(curl);

			stringstream response;
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_stringstream_writer);

			res = curl_easy_perform(curl);

			string response_str;
			try {
				boost::iostreams::filtering_istream decompress_stream;
				decompress_stream.push(boost::iostreams::gzip_decompressor());
				decompress_stream.push(response);

				response_str = string(istreambuf_iterator<char>(decompress_stream), {});
			} catch (...) {
				curl_easy_cleanup(curl);
				error = ERROR;
				return "";
			}


			if (res == CURLE_OK) {
				long response_code;
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
				if (response_code == 200) {
					error = OK;
				}
			}

			curl_easy_cleanup(curl);

			return response_str;
		}

		return "";
	}

	void file_to_stream(const string &file_path, ostream &output_stream, int &error) {
		CURL *curl = curl_easy_init();
		error = ERROR;
		if (curl) {
			CURLcode res;
			LOG_INFO("Downloading url: " + make_url(file_path));
			curl_easy_setopt(curl, CURLOPT_URL, make_url(file_path).c_str());

			set_internal_auth(curl);

			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output_stream);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_ostream_writer);

			res = curl_easy_perform(curl);

			if (res == CURLE_OK) {
				long response_code;
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
				if (response_code == 200) {
					error = OK;
				}
			}

			curl_easy_cleanup(curl);

		}
	}

	void gz_file_to_stream(const string &file_path, ostream &output_stream, int &error) {
		CURL *curl = curl_easy_init();
		error = ERROR;
		if (curl) {
			CURLcode res;
			LOG_INFO("Downloading url: " + make_url(file_path));
			curl_easy_setopt(curl, CURLOPT_URL, make_url(file_path).c_str());

			set_internal_auth(curl);

			stringstream response;
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_stringstream_writer);

			res = curl_easy_perform(curl);

			if (res == CURLE_OK) {
				long response_code;
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
				if (response_code == 200) {
					error = OK;
				}
			}

			try {
				boost::iostreams::filtering_istream decompress_stream;
				decompress_stream.push(boost::iostreams::gzip_decompressor());
				decompress_stream.push(response);

				output_stream << decompress_stream.rdbuf();
			} catch(...) {
				error = ERROR;
			}

			curl_easy_cleanup(curl);
		}
	}

	void url_to_string(const string &url, string &buffer, int &error) {
		CURL *curl = curl_easy_init();
		error = ERROR;
		const size_t original_buffer_size = buffer.size();
		if (curl) {
			CURLcode res;
			LOG_INFO("Downloading url: " + url);
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 5000);
			curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 5);

			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_string_writer);

			res = curl_easy_perform(curl);

			if (res == CURLE_OK) {
				long response_code;
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
				if (response_code >= 200 && response_code < 300) {
					error = OK;
				}
			} else {
				// If an error ocurred we set the size of the buffer to the original size, removing any appended data.
				buffer.resize(original_buffer_size);
			}

			curl_easy_cleanup(curl);
		}
	}

	string run_gz_download_thread(const string &file_path) {
		size_t hsh = algorithm::hash(file_path);
		const string target_filename = "/mnt/" + to_string(hsh % 8) + "/tmp/tmp_" + to_string(hsh);
		ofstream target_file(target_filename, ios::binary | ios::trunc);
		int error;
		gz_file_to_stream(file_path, target_file, error);
		if (error != OK) {
			return "";
		}
		return target_filename;
	}

	vector<string> download_gz_files_to_disk(const vector<string> &files_to_download) {
		
		ThreadPool pool(config::num_async_file_transfers);
		std::vector<std::future<string>> results;

		for (const string &file : files_to_download) {
			results.emplace_back(
				pool.enqueue([file] {
					return run_gz_download_thread(file);
				})
			);
		}

		vector<string> local_filenames;
		for(auto && result: results) {
			const string filename = result.get();
			if (filename != "") {
				local_filenames.push_back(filename);
			}
		}

		return local_filenames;
	}

	void delete_downloaded_files(const vector<string> &files) {
		LOG_INFO("Deleting " + to_string(files.size()) + " downloaded files");
		for (const string &file : files) {
			file::delete_file(file);
		}
	}

	size_t head_content_length(const string &url, int &error) {
		CURL *curl = curl_easy_init();
		error = ERROR;
		if (curl) {
			CURLcode res;
			LOG_INFO("Making head request to:" + url);
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

			stringstream response;
			curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
			curl_easy_setopt(curl, CURLOPT_HEADER, 1);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_stringstream_writer);

			res = curl_easy_perform(curl);

			string response_str;
			try {
				response_str = string(istreambuf_iterator<char>(response), {});
			} catch (...) {
				curl_easy_cleanup(curl);
				error = ERROR;
				return 0;
			}

			if (res == CURLE_OK) {
				long response_code;
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
				if (response_code == 200) {
					error = OK;
				} else {
					curl_easy_cleanup(curl);
					return 0;
				}
			}

			curl_easy_cleanup(curl);

			const string content_len_str = parser::get_http_header(text::lower_case(response_str), "content-length: ");
			size_t content_len;
			try {
				content_len = stoull(content_len_str);
			} catch (...) {
				error = ERROR;
				return 0;
			}

			return content_len;
		}

		return 0;
	}

	int upload_file(const string &path, const string &data) {
		CURL *curl = curl_easy_init();
		if (curl) {
			CURLcode res;
			const string url = "http://" + config::upload + "/" + path;
			LOG_INFO("Uploading file to:" + url);
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30L);
			curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 30L);

			struct curl_string_read_struct arg;
			arg.buffer = data.c_str();
			arg.buffer_len = data.size();
			arg.offset = 0;

			curl_easy_setopt(curl, CURLOPT_UPLOAD, 1l);
			curl_easy_setopt(curl, CURLOPT_USERNAME, config::file_upload_user.c_str());
			curl_easy_setopt(curl, CURLOPT_PASSWORD, config::file_upload_password.c_str());
			curl_easy_setopt(curl, CURLOPT_READFUNCTION, curl_string_reader);
			curl_easy_setopt(curl, CURLOPT_READDATA, &arg);

			res = curl_easy_perform(curl);

			curl_easy_cleanup(curl);

			if (res == CURLE_OK) {
				return OK;
			}
			return ERROR;
		}

		return ERROR;
	}

	int upload_gz_file(const string &path, const string &data) {
		CURL *curl = curl_easy_init();
		if (curl) {
			CURLcode res;
			const string url = "http://" + config::upload + "/" + path;
			LOG_INFO("Uploading file to:" + url);
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30L);
			curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 30L);

			stringstream ss(data);
			boost::iostreams::filtering_istream compress_stream;
			compress_stream.push(boost::iostreams::gzip_compressor());
			compress_stream.push(ss);

			string compressed_data = string(istreambuf_iterator<char>(compress_stream), {});

			struct curl_string_read_struct arg;
			arg.buffer = compressed_data.c_str();
			arg.buffer_len = compressed_data.size();
			arg.offset = 0;

			curl_easy_setopt(curl, CURLOPT_UPLOAD, 1l);
			curl_easy_setopt(curl, CURLOPT_USERNAME, config::file_upload_user.c_str());
			curl_easy_setopt(curl, CURLOPT_PASSWORD, config::file_upload_password.c_str());
			curl_easy_setopt(curl, CURLOPT_READFUNCTION, curl_string_reader);
			curl_easy_setopt(curl, CURLOPT_READDATA, &arg);

			res = curl_easy_perform(curl);

			curl_easy_cleanup(curl);

			if (res == CURLE_OK) {
				return OK;
			}
			return ERROR;
		}

		return ERROR;
	}

	int upload_file_from_disk(const string &dest_path, const string &filename) {
		CURL *curl = curl_easy_init();
		if (curl) {
			CURLcode res;
			const string url = "http://" + config::upload + "/" + dest_path;
			LOG_INFO("Uploading file to:" + url);
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30L);
			curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 30L);

			std::ifstream infile(filename, std::ios::in | std::ios::binary);

			curl_easy_setopt(curl, CURLOPT_UPLOAD, 1l);
			curl_easy_setopt(curl, CURLOPT_USERNAME, config::file_upload_user.c_str());
			curl_easy_setopt(curl, CURLOPT_PASSWORD, config::file_upload_password.c_str());
			curl_easy_setopt(curl, CURLOPT_READFUNCTION, curl_file_reader);
			curl_easy_setopt(curl, CURLOPT_READDATA, &infile);

			res = curl_easy_perform(curl);

			curl_easy_cleanup(curl);

			if (res == CURLE_OK) {
				return OK;
			}
			return ERROR;
		}

		return ERROR;
	}

	/*
	 * Perform simple GET request and return response.
	 * */
	http::response get(const string &url) {
		return get(url, vector<string>{});
	}

	http::response get(const string &url, const vector<string> &headers) {
		CURL *curl = curl_easy_init();
		struct curl_slist *header_list = NULL;
		http::response response;
		if (curl) {

			for (const string &header : headers) {
				header_list = curl_slist_append(header_list, header.c_str());
			}

			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

			curl_easy_setopt(curl, CURLOPT_USERNAME, config::file_upload_user.c_str());
			curl_easy_setopt(curl, CURLOPT_PASSWORD, config::file_upload_password.c_str());
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);

			stringstream response_stream;
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_stream);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_stringstream_writer);

			curl_easy_perform(curl);

			curl_slist_free_all(header_list);

			size_t code = 0;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

			response.code(code);
			response.body(response_stream.str());

			curl_easy_cleanup(curl);
		}

		return response;
	}

	/*
	 * Perform simple POST request and return response.
	 * */
	http::response post(const string &url, const string &data) {
		return post(url, data, {});
	}

	http::response post(const string &url, const string &data, const vector<string> &headers) {
		CURL *curl = curl_easy_init();
		struct curl_slist *header_list = NULL;
		http::response response;
		if (curl) {

			for (const string &header : headers) {
				header_list = curl_slist_append(header_list, header.c_str());
			}

			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

			struct curl_string_read_struct arg;
			arg.buffer = data.c_str();
			arg.buffer_len = data.size();
			arg.offset = 0;

			curl_easy_setopt(curl, CURLOPT_POST, 1l);
			curl_easy_setopt(curl, CURLOPT_USERNAME, config::file_upload_user.c_str());
			curl_easy_setopt(curl, CURLOPT_PASSWORD, config::file_upload_password.c_str());
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
			curl_easy_setopt(curl, CURLOPT_READFUNCTION, curl_string_reader);
			curl_easy_setopt(curl, CURLOPT_READDATA, &arg);

			stringstream response_stream;
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_stream);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_stringstream_writer);

			CURLcode curl_result = curl_easy_perform(curl);

			if (curl_result == CURLE_OK) {
				size_t code = 0;
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
				response.code(code);
				response.body(response_stream.str());
			} else {
				response.code(0);
				response.body("");
			}

			curl_easy_cleanup(curl);
		}

		return response;
	}

	/*
	 * Perform simple PUT request and return response.
	 * */
	http::response put(const string &url, const string &data) {
		CURL *curl = curl_easy_init();
		http::response response;
		if (curl) {
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30L);
			curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 30L);

			struct curl_string_read_struct arg;
			arg.buffer = data.c_str();
			arg.buffer_len = data.size();
			arg.offset = 0;

			curl_easy_setopt(curl, CURLOPT_UPLOAD, 1l);
			curl_easy_setopt(curl, CURLOPT_USERNAME, config::file_upload_user.c_str());
			curl_easy_setopt(curl, CURLOPT_PASSWORD, config::file_upload_password.c_str());
			curl_easy_setopt(curl, CURLOPT_READFUNCTION, curl_string_reader);
			curl_easy_setopt(curl, CURLOPT_READDATA, &arg);

			stringstream response_stream;
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_stream);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_stringstream_writer);

			curl_easy_perform(curl);

			size_t code = 0;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

			response.code(code);
			response.body(response_stream.str());

			curl_easy_cleanup(curl);
		}

		return response;
	}
}
