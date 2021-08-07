
#include "Transfer.h"

namespace Transfer {

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

	void prepare_curl(CURL *curl) {
		curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());
		curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
	}

	string make_url(const string &file_path) {
		if (file_path.size() && file_path[0] != '/') {
			return file_server + "/" + file_path;
		}
		return file_server + file_path;
	}

	string file_to_string(const string &file_path, int &error) {
		CURL *curl = curl_easy_init();
		error = ERROR;
		if (curl) {
			CURLcode res;
			curl_easy_setopt(curl, CURLOPT_URL, make_url(file_path).c_str());

			prepare_curl(curl);

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
			curl_easy_setopt(curl, CURLOPT_URL, make_url(file_path).c_str());

			prepare_curl(curl);

			stringstream response;
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_stringstream_writer);

			res = curl_easy_perform(curl);

			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(response);

			const string response_str = string(istreambuf_iterator<char>(decompress_stream), {});

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
			curl_easy_setopt(curl, CURLOPT_URL, make_url(file_path).c_str());

			prepare_curl(curl);

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
			curl_easy_setopt(curl, CURLOPT_URL, make_url(file_path).c_str());

			prepare_curl(curl);

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

			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(response);

			output_stream << decompress_stream.rdbuf();

			curl_easy_cleanup(curl);
		}
	}
}
