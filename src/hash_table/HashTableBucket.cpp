
#include "HashTableBucket.h"

HashTableBucket::HashTableBucket(size_t bucket_id, const vector<size_t> &shard_ids)
: m_bucket_id(bucket_id), m_shard_ids(shard_ids)
{
	m_first_shard_id = shard_ids[0];
	m_last_shard_id = shard_ids.back();

	m_port = HT_PORT_START + bucket_id;
	m_thread = new thread(&HashTableBucket::run_server, this);
}

HashTableBucket::~HashTableBucket() {
	HashTableMessage message;
	message.m_message_type = HT_MESSAGE_STOP;
	HashTableMessage response = send_message(message);
	m_thread->join();
}

void HashTableBucket::add(uint64_t key, const string &value) {
	HashTableMessage message;
	message.m_message_type = HT_MESSAGE_ADD;
	message.m_key = key;
	size_t str_len = min(value.size(), (size_t)HT_DATA_LENGTH);
	memcpy(message.m_data, value.c_str(), str_len);
	message.m_data[str_len] = '\0';
	HashTableMessage response = send_message(message);
}

string HashTableBucket::find(uint64_t key) {
	HashTableMessage message;
	message.m_message_type = HT_MESSAGE_FIND;
	message.m_key = key;
	HashTableMessage response = send_message(message);
	return string(response.m_data);
}

void HashTableBucket::run_server() {

	load_shards();

	if ((m_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	int opt = 1;

	if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in address;
	int addr_len = sizeof(address);
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(m_port);

	if (bind(m_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	if (listen(m_socket, 3) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	int new_socket;

	while ((new_socket = accept(m_socket, (struct sockaddr *)&address, (socklen_t*)&addr_len))) {
		if (new_socket < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}

		if (!read_socket(new_socket)) {
			break;
		}
		close(new_socket);
	}

	close_shards();

	HashTableMessage response;
	send(new_socket, &response , sizeof(response), 0);

	close(new_socket);
	close(m_socket);
}

HashTableMessage HashTableBucket::send_message(const HashTableMessage &message) {

	int send_socket;
	struct sockaddr_in serv_addr;

	if ((send_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Send socket creation error \n");
		cout << string("Failed to create socket errno: ") + to_string(errno) << endl;
		exit(EXIT_FAILURE);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port());

	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
		printf("\nInvalid address/ Address not supported \n");
		exit(EXIT_FAILURE);
	}

	if (connect(send_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("\nConnection Failed \n");
		cout << string("Failed to connect errno: ") + to_string(errno) << endl;
		exit(EXIT_FAILURE);
	}

	send(send_socket, &message, sizeof(message), 0);

	// Read response.
	HashTableMessage response;
	int valread = read(send_socket, &response, sizeof(response));

	close(send_socket);

	return response;
}

bool HashTableBucket::read_socket(int socket) {

	// Receive a message from client

	HashTableMessage message;

	recv(socket, &message, sizeof(message), 0);

	HashTableMessage response;
	response.m_message_type = message.m_message_type;

	char *response_data = NULL;
	if (message.m_message_type == HT_MESSAGE_ADD) {
		m_shards[message.m_key % HT_NUM_SHARDS]->add(message.m_key, message.m_data);
	} else if (message.m_message_type == HT_MESSAGE_FIND) {

		string result = m_shards[message.m_key % HT_NUM_SHARDS]->find(message.m_key);

		size_t str_len = min(result.size(), (size_t)HT_DATA_LENGTH);
		memcpy(response.m_data, result.c_str(), str_len);
		result[str_len] = '\0';

	} else if (message.m_message_type == HT_MESSAGE_STOP) {
		return false;
	}

	// Send response.
	send(socket, &response , sizeof(response), 0);

	return true;
}

void HashTableBucket::load_shards() {
	for (const size_t shard_id : m_shard_ids) {
		m_shards[shard_id] = new HashTableShard(shard_id);
	}
}

void HashTableBucket::close_shards() {
	for (auto &iter : m_shards) {
		delete iter.second;
	}
}

