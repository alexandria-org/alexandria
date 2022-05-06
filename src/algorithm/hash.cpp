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

#include "hash.h"

namespace algorithm {

	/*
	 * Murmur hash by Austin Appleby
	 * Taken from here https://sites.google.com/site/murmurhash/
	 * */
	size_t murmur_hash(const char *key, size_t len, size_t seed) {
		const uint64_t m = 0xc6a4a7935bd1e995ull;
		const int r = 47;

		uint64_t h = seed ^ (len * m);

		const uint64_t * data = (const uint64_t *)key;
		const uint64_t * end = data + (len/8);

		while(data != end) {
			uint64_t k = *data++;

			k *= m; 
			k ^= k >> r; 
			k *= m; 
			
			h ^= k;
			h *= m; 
		}

		const unsigned char * data2 = (const unsigned char*)data;

		switch(len & 7) {
			case 7: h ^= uint64_t(data2[6]) << 48;
			case 6: h ^= uint64_t(data2[5]) << 40;
			case 5: h ^= uint64_t(data2[4]) << 32;
			case 4: h ^= uint64_t(data2[3]) << 24;
			case 3: h ^= uint64_t(data2[2]) << 16;
			case 2: h ^= uint64_t(data2[1]) << 8;
			case 1: h ^= uint64_t(data2[0]);
				h *= m;
		};
 
		h ^= h >> r;
		h *= m;
		h ^= h >> r;

		return h;
	}

	/*
	 * This is a modified version of the above murmur_hash. Used for our robustBF
	 * https://github.com/patgiri/robustBF/blob/main/murmur.h
	 * */
	unsigned int murmur_hash2(const char *key, size_t len, size_t seed) {
		// 'm' and 'r' are mixing constants generated offline.
		// They're not really 'magic', they just happen to work well.

		const unsigned int m = 0x5bd1e995;
		const int r = 24;

		// Initialize the hash to a 'random' value

		unsigned int h = seed ^ len;

		// Mix 4 bytes at a time into the hash

		const unsigned char * data = (const unsigned char *)key;

		while(len >= 7)
		{
			unsigned int k = *(unsigned int *)data;

			k *= m; 
			k ^= k >> r; 
			k *= m; 
			
			h *= m; 
			h ^= k;

			data += 7;
			len -= 7;
		}
		
		// Handle the last few bytes of the input array

		switch(len)
		{
		case 3: h ^= data[2] << 16;
		case 2: h ^= data[1] << 8;
		case 1: h ^= data[0];
				h *= m;
		};

		// Do a few final mixes of the hash to ensure the last few
		// bytes are well-incorporated.

		h ^= h >> 13;
		h *= m;
		h ^= h >> 15;

		return h;
	}

	size_t hash(const std::string &str) {
		static const size_t seed = 0xc70f6907ul;
		return murmur_hash(str.c_str(), str.size(), seed);
	}

	size_t hash_with_seed(const std::string &str, size_t seed) {
		return murmur_hash2(str.c_str(), str.size(), seed);
	}


}
