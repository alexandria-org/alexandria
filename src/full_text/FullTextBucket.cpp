
#include "FullTextBucket.h"


FullTextBucket::FullTextBucket(const string &db_name, size_t bucket_id, const vector<size_t> &shard_ids)
: m_db_name(db_name), m_bucket_id(bucket_id), m_shard_ids(shard_ids)
{
	
	m_first_shard_id = shard_ids[0];
	m_last_shard_id = shard_ids.back();

	m_port = FT_PORT_START + bucket_id;
	m_thread = new thread(&FullTextBucket::run_server, this);
}

FullTextBucket::~FullTextBucket() {
	FullTextBucketMessage message;
	message.m_message_type = FT_MESSAGE_STOP;
	FullTextBucketMessage response = send_message(message);
	m_thread->join();
}

void FullTextBucket::add(uint64_t key, uint64_t value, uint32_t score) {
	FullTextBucketMessage message;
	message.m_message_type = FT_MESSAGE_ADD;
	message.m_key = key;
	message.m_value = value;
	message.m_score = score;
	FullTextBucketMessage response = send_message(message);
}

void FullTextBucket::add_file(const string &file_name, const vector<size_t> &cols, const vector<uint32_t> &scores) {
	FullTextBucketMessage message;
	message.m_message_type = FT_MESSAGE_ADD_FILE;
	message.store_file_data(file_name, cols, scores);
	FullTextBucketMessage response = send_message(message);
}

void FullTextBucket::sort_cache() {
	FullTextBucketMessage message;
	message.m_message_type = FT_MESSAGE_SORT;
	FullTextBucketMessage response = send_message(message);
}

vector<FullTextResult> FullTextBucket::find(uint64_t key) {
	FullTextBucketMessage message;
	message.m_message_type = FT_MESSAGE_FIND;
	message.m_key = key;
	FullTextBucketMessage response = send_message(message);
	return response.result_vector();
}

void FullTextBucket::save_file() {
	FullTextBucketMessage message;
	message.m_message_type = FT_MESSAGE_SAVE;
	FullTextBucketMessage response = send_message(message);
}

void FullTextBucket::truncate() {
	FullTextBucketMessage message;
	message.m_message_type = FT_MESSAGE_TRUNCATE;
	FullTextBucketMessage response = send_message(message);
}

size_t FullTextBucket::disk_size() {
	FullTextBucketMessage message;
	message.m_message_type = FT_MESSAGE_DISK_SIZE;
	FullTextBucketMessage response = send_message(message);
	return response.m_size_response;
}

size_t FullTextBucket::cache_size() {
	FullTextBucketMessage message;
	message.m_message_type = FT_MESSAGE_CACHE_SIZE;
	FullTextBucketMessage response = send_message(message);
	return response.m_size_response;
}

void FullTextBucket::run_server() {

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

	FullTextBucketMessage response;
	send(new_socket, &response , sizeof(response), 0);

	close(new_socket);
	close(m_socket);
}

FullTextBucketMessage FullTextBucket::send_message(const FullTextBucketMessage &message) {
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

	if (message.m_data_size) {
		send(send_socket, message.data(), message.m_data_size, 0);
	}

	// Read response.
	FullTextBucketMessage response;
	int valread = read(send_socket, &response, sizeof(response));
	response.allocate_data();

	valread = read(send_socket, response.data(), response.m_data_size);

	close(send_socket);

	return response;
}

bool FullTextBucket::read_socket(int socket) {

	// Receive a message from client
	// First size_t must contain a size_t containing the size of the message.

	FullTextBucketMessage message;

	recv(socket, &message, sizeof(message), 0);

	if (message.m_data_size) {
		message.allocate_data();	
		recv(socket, message.data(), message.m_data_size, 0);
	}

	vector<FullTextResult> results;
	FullTextBucketMessage response;
	response.m_message_type = message.m_message_type;

	char *response_data = NULL;
	if (message.m_message_type == FT_MESSAGE_ADD) {
		m_shards[message.m_key % FT_NUM_SHARDS]->add(message.m_key, message.m_value, message.m_score);
	} else if (message.m_message_type == FT_MESSAGE_SORT) {
		for (auto &iter : m_shards) {
			iter.second->sort_cache();
		}
	} else if (message.m_message_type == FT_MESSAGE_FIND) {

		results = m_shards[message.m_key % FT_NUM_SHARDS]->find(message.m_key);

		response.m_data_size = sizeof(FullTextResult) * results.size();
		response_data = (char *)results.data();
	} else if (message.m_message_type == FT_MESSAGE_SAVE) {
		for (auto &iter : m_shards) {
			iter.second->save_file();
		}
	} else if (message.m_message_type == FT_MESSAGE_TRUNCATE) {
		for (auto &iter : m_shards) {
			iter.second->truncate();
		}
	} else if (message.m_message_type == FT_MESSAGE_STOP) {
		return false;
	} else if (message.m_message_type == FT_MESSAGE_DISK_SIZE) {
		response.m_size_response = 0;
		for (auto &iter : m_shards) {
			response.m_size_response += iter.second->disk_size();
		}
	} else if (message.m_message_type == FT_MESSAGE_CACHE_SIZE) {
		response.m_size_response = 0;
		for (auto &iter : m_shards) {
			response.m_size_response += iter.second->cache_size();
		}
	} else if (message.m_message_type == FT_MESSAGE_ADD_FILE) {
		string file_name;
		vector<size_t> cols;
		vector<uint32_t> scores;
		message.read_file_data(file_name, cols, scores);
		add_file_to_shards(file_name, cols, scores);
	}
	
	// Send response.
	send(socket, &response , sizeof(response), 0);

	if (response_data != NULL) {
		send(socket, response_data, response.m_data_size, 0);
	}

	return true;
}

void FullTextBucket::load_shards() {
	for (const size_t shard_id : m_shard_ids) {
		m_shards[shard_id] = new FullTextShard(m_db_name, shard_id);
	}
}

void FullTextBucket::close_shards() {
	for (auto &iter : m_shards) {
		iter.second->save_file();
		delete iter.second;
	}
}

void FullTextBucket::add_file_to_shards(const string &file_name, const vector<size_t> &cols,
	const vector<uint32_t> &scores) {

	using namespace boost::iostreams;

	ifstream file(file_name);
	if (!file.is_open()) {
		// Handle error here.
		return;
	}

	filtering_istream decompress_stream;
	decompress_stream.push(gzip_decompressor());
	decompress_stream.push(file);

	string line;
	while (getline(decompress_stream, line)) {
		vector<string> col_values;
		boost::algorithm::split(col_values, line, boost::is_any_of("\t"));
		size_t score_index = 0;
		for (size_t col_index : cols) {
			add_data_to_shards(col_values[0], col_values[col_index], scores[score_index]);
			score_index++;
		}
	}

	// sort shards.
	for (auto &iter : m_shards) {
		iter.second->sort_cache();
	}
}

void FullTextBucket::add_data_to_shards(const string &key, const string &text, uint32_t score) {

	uint64_t key_hash = m_hasher(key);

	vector<string> words = get_full_text_words(text);
	for (const string &word : words) {
		uint64_t word_hash = m_hasher(word);

		const size_t shard_id = word_hash % FT_NUM_SHARDS;

		if ((shard_id >= m_first_shard_id) && (shard_id <= m_last_shard_id)) {
			m_shards[shard_id]->add(key_hash, word_hash, score);
		}
	}
}