
#include "HashTable.h"

HashTable::HashTable() {
	for (size_t bucket_id = 0; bucket_id < HT_NUM_BUCKETS; bucket_id++) {
		m_buckets.push_back(new HashTableBucket(bucket_id, shard_ids_for_bucket(bucket_id)));
	}
}

HashTable::~HashTable() {

	for (HashTableBucket *bucket : m_buckets) {
		delete bucket;
	}
}

void HashTable::add(uint64_t key, const string &value) {

	HashTableBucket *bucket = bucket_for_hash(key);
	bucket->add(key, value);

}

string HashTable::find(uint64_t key) {
	HashTableBucket *bucket = bucket_for_hash(key);
	return bucket->find(key);
}

void HashTable::wait_for_start() {
	usleep(500000);
}

vector<size_t> HashTable::shard_ids_for_bucket(size_t bucket_id) {
	const size_t shards_per_bucket = HT_NUM_SHARDS / HT_NUM_BUCKETS;

	const size_t start = bucket_id * shards_per_bucket;
	const size_t end = start + shards_per_bucket;
	
	vector<size_t> shard_ids;
	for (size_t shard_id = start; shard_id < end; shard_id++) {
		shard_ids.push_back(shard_id);
	}

	return shard_ids;
}

HashTableBucket *HashTable::bucket_for_hash(uint64_t hash) {
	const size_t shards_per_bucket = HT_NUM_SHARDS / HT_NUM_BUCKETS;
	size_t shard_id = hash % HT_NUM_SHARDS;
	size_t bucket_id = shard_id / shards_per_bucket;
	return m_buckets[bucket_id];
}

