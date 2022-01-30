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
#include "datetime.h"
#include <ctime>

namespace System {

	size_t cur_date() {
		time_t tt = time(NULL);
		struct tm tm = *localtime(&tt);
		size_t year_since_00 = tm.tm_year - 100;
		size_t year = 2000 + year_since_00;
		return (year * 100 * 100) + ((tm.tm_mon + 1) * 100) + tm.tm_mday;
	}

	size_t cur_time() {
		time_t tt = time(NULL);
		struct tm tm = *localtime(&tt);
		return (tm.tm_hour * 100 * 100) + (tm.tm_min * 100) + tm.tm_sec;
	}

	size_t cur_datetime() {
		size_t date = cur_date();
		return (date * 100 * 100 * 100) + cur_time();
	}

}
