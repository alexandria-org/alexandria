
#pragma once

#include <iostream>

#define IS_MULTIBYTE_CODEPOINT(ch) (((unsigned char)ch >> 7) && !(((unsigned char)ch >> 6) & 0x1))
#define IS_UTF8_START_1(ch) (((unsigned char)ch >> 5) == 0b00000110 && ((unsigned char)ch & 0b00011111) >= 0b00000010)
#define IS_UTF8_START_2(ch) (((unsigned char)ch >> 4) == 0b00001110)
#define IS_UTF8_START_3(ch) (((unsigned char)ch >> 3) == 0b00011110)
#define IS_UNKNOWN_UTF8_START(ch) (ch >> 7)

class Unicode {

public:
	
	static std::string encode(const std::string &str);
	static bool is_valid(const std::string &str);

};