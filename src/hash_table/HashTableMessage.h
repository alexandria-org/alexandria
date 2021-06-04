
#pragma once

#include "HashTable.h"

class HashTableMessage {

public:

	size_t m_message_type;
	uint64_t m_key;
	char data[HT_DATA_LENGTH];

};
