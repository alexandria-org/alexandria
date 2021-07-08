
#pragma once

#define DOMAIN_ADJUSTMENT 0x1
#define URL_ADJUSTMENT 0x2

struct Adjustment {

	unsigned char type;
	uint64_t word_hash;
	uint64_t key_hash;
	uint32_t score;
	uint32_t domain_harmonic;
	
};

