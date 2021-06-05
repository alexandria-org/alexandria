
#pragma once

#include "HashTable.h"

class HashTableMessage {

public:

	size_t m_message_type;
	uint64_t m_key;
	char m_data[HT_DATA_LENGTH + 1];

};
