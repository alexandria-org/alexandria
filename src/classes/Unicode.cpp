
#include "Unicode.h"


std::string Unicode::encode(const std::string &str) {

	const char *cstr = str.c_str();
	size_t len = str.size();

	char *target = new char[str.size()];

	size_t last_unicode = len;
	size_t utf8_len = 0;
	for (size_t i = 0; i < len; i++) {
		bool copy = true;
		if (utf8_len == 0) {
			if (IS_UTF8_START_1(cstr[i])) {
				utf8_len = 1;
				last_unicode = i;
			} else if (IS_UTF8_START_2(cstr[i])) {
				utf8_len = 2;
				last_unicode = i;
			} else if (IS_UTF8_START_3(cstr[i])) {
				utf8_len = 3;
				last_unicode = i;
			} else if (IS_UNKNOWN_UTF8_START(cstr[i])) {
				copy = false;
			} else if ('\x00' <= cstr[i] && cstr[i] <= '\x1f') {
				copy = false;
			}
		} else if (IS_MULTIBYTE_CODEPOINT(cstr[i])) {
			utf8_len--;
		} else {
			// This unicode character has been terminated too soon.
			copy = false;
			for (size_t j = last_unicode; j <= i; j++) {
				target[j] = '?';
			}
			utf8_len = 0;
		}
		if (copy) {
			target[i] = cstr[i];
		} else {
			target[i] = '?';
		}
	}

	std::string ret(target, len);
	delete target;
	if (utf8_len) {
		return ret.substr(0, last_unicode);
	} else {
		return ret;
	}
}

bool Unicode::is_valid(const std::string &str) {
	
	const char *cstr = str.c_str();
	size_t len = str.size();

	size_t last_unicode = len;
	size_t utf8_len = 0;
	for (size_t i = 0; i < len; i++) {
		if (utf8_len == 0) {
			if (IS_UTF8_START_1(cstr[i])) {
				utf8_len = 1;
				last_unicode = i;
			} else if (IS_UTF8_START_2(cstr[i])) {
				utf8_len = 2;
				last_unicode = i;
			} else if (IS_UTF8_START_3(cstr[i])) {
				utf8_len = 3;
				last_unicode = i;
			} else if (IS_UNKNOWN_UTF8_START(cstr[i])) {
				return false;
			}
		} else if (IS_MULTIBYTE_CODEPOINT(cstr[i])) {
			utf8_len--;
		} else {
			// This unicode character has been terminated too soon.
			return false;
		}
	}

	if (utf8_len) {
		return false;
	}

	return true;
}
