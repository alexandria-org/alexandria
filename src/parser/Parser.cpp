
#include "parser.h"

namespace Parser {

	bool is_percent_encoding(const char *cstr) {
		const char first = tolower(cstr[1]);
		const char second = tolower(cstr[2]);
		const bool first_valid = (first >= '0' && first <= '9') || (first >= 'a' && first <= 'f');
		const bool second_valid = (second >= '0' && second <= '9') || (second >= 'a' && second <= 'f');
		return cstr[0] == '%' && first_valid && second_valid;
	}

	string urldecode(const string &str) {
		const size_t len = str.size();
		const char *cstr = str.c_str();
		char *ret = new char[len + 1];
		size_t j = 0;
		for (size_t i = 0; i < len; i++) {
			if (i < len - 2 && is_percent_encoding(&cstr[i])) {
				ret[j++] = (char)stoi(string(&cstr[i + 1], 2), NULL, 16);
				i += 2;
			} else if (i < len - 1 && cstr[i] == '%' && cstr[i + 1] == '%') {
				ret[j++] = '%';
				i++;
			} else {
				ret[j++] = cstr[i];
			}
		}
		ret[j] = '\0';

		string ret_str(ret);

		delete ret;

		return ret_str;
	}
}
