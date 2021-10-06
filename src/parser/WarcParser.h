
#pragma once

namespace WarcParser {

	struct Response {
		bool status;
		string status_str;
		string result;
		string links;
	};

	Response *parse(const string &bucket, const string &key);
	Response *parse(const string &bucket, const string &key);

	void delete_response(Response *res);

}
