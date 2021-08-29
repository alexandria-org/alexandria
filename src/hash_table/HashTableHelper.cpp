
#include "config.h"
#include "HashTableHelper.h"
#include "system/Logger.h"

namespace HashTableHelper {

	void truncate(const string &hash_table_name) {
		vector<HashTableShardBuilder *> shards = create_shard_builders(hash_table_name);

		for (HashTableShardBuilder *shard : shards) {
			shard->truncate();
		}

		delete_shard_builders(shards);
	}

	vector<HashTableShardBuilder *> create_shard_builders(const string &hash_table_name) {
		vector<HashTableShardBuilder *> shards;
		for (size_t shard_id = 0; shard_id < Config::ht_num_shards; shard_id++) {
			shards.push_back(new HashTableShardBuilder(hash_table_name, shard_id));
		}

		return shards;
	}

	void delete_shard_builders(vector<HashTableShardBuilder *> &shards) {
		for (HashTableShardBuilder *shard : shards) {
			delete shard;
		}

		shards.clear();
	}

	void add_data(vector<HashTableShardBuilder *> &shards, uint64_t key, const string &value) {
		shards[key % Config::ht_num_shards]->add(key, value);
	}

	void write(vector<HashTableShardBuilder *> &shards) {
		for (HashTableShardBuilder *shard : shards) {
			shard->write();
		}
	}

	void sort(vector<HashTableShardBuilder *> &shards) {
		for (HashTableShardBuilder *shard : shards) {
			shard->sort();
		}
	}

	void optimize(vector<HashTableShardBuilder *> &shards) {
		for (HashTableShardBuilder *shard : shards) {
			LogInfo("Optimizing shard: " + shard->filename_data());
			shard->optimize();
			break;
		}
	}

}
