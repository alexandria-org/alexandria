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

#pragma once

#include <iostream>

#define IS_MULTIBYTE_CODEPOINT(ch) (((unsigned char)ch >> 7) && !(((unsigned char)ch >> 6) & 0x1))
#define IS_UTF8_START_1(ch) (((unsigned char)ch >> 5) == 0b00000110 && ((unsigned char)ch & 0b00011111) >= 0b00000010)
#define IS_UTF8_START_2(ch) (((unsigned char)ch >> 4) == 0b00001110)
#define IS_UTF8_START_3(ch) (((unsigned char)ch >> 3) == 0b00011110)
#define IS_UNKNOWN_UTF8_START(ch) (ch >> 7)

namespace parser {

	class unicode {

		public:
			
			static std::string encode(const std::string &str);
			static bool is_valid(const std::string &str);

	};

}
