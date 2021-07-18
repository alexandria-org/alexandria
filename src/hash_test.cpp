
#include <iostream>
#include "system/SubSystem.h"
#include "parser/URL.h"

int main() {

	string cc_batch = "CC-MAIN-2021-17";

	SubSystem *sub_system = new SubSystem();

	string warc_paths_url = string("crawl-data/") + cc_batch + "/warc.paths.gz";
	TsvFileS3 warc_paths_file(sub_system->s3_client(), "commoncrawl", warc_paths_url);

	vector<string> warc_paths_raw;
	warc_paths_file.read_column_into(0, warc_paths_raw);

	unordered_map<uint64_t, string> hash_to_url;
	unordered_map<string, uint64_t> url_to_hash;

	unordered_map<uint64_t, string> hash_to_host;
	unordered_map<string, uint64_t> host_to_hash;

	double probability_for_collision = 0.0;
	double probability_for_domain_collision = 0.0;
	size_t checked = 0;
	size_t collisions = 0;
	size_t domain_collisions = 0;

	hash<string> hasher;

	for (string warc_path : warc_paths_raw) {
		warc_path.replace(warc_path.find(".warc.gz"), 8, ".gz");
		vector<string> urls;
		try {
			TsvFileS3 warc_file(sub_system->s3_client(), "alexandria-cc-output", warc_path);
			warc_file.read_column_into(0, urls);
		} catch (runtime_error &err) {
			continue;
		}
		cout << "probability_for_collision: " << probability_for_collision << " checked: " << checked << " domains checked: " << host_to_hash.size()
		<< " probability_for_domain_collision: " << probability_for_domain_collision
		<< endl;


		for (const string &url_str : urls) {
			URL url(url_str);
			uint64_t real_hash = hasher(url_str);
			uint64_t host_hash = url.host_hash();
			const size_t host_bits = 20;
			uint64_t host_part = host_hash << (64 - host_bits);
			uint64_t hash = (real_hash >> host_bits) | host_part;
			string host_str = url.host();
			checked++;
			if (hash_to_url.find(hash) != hash_to_url.end() && url_to_hash.find(url_str) == url_to_hash.end()) {
				collisions++;
				cout << "Found real collision for:" << endl;
				cout << "\t" << url_str << endl;
				cout << "\t" << hash_to_url[hash] << endl << endl;

				probability_for_collision = ((double)collisions) / checked;
			}
			if (hash_to_host.find(host_part) != hash_to_host.end() && host_to_hash.find(host_str) == host_to_hash.end()) {
				domain_collisions++;
				/*cout << "Found domain collision for:" << endl;
				cout << "\t" << host_str << endl;
				cout << "\t" << hash_to_host[host_part] << endl << endl;*/

				probability_for_domain_collision = ((double)domain_collisions) / host_to_hash.size();
			}
			hash_to_url[hash] = url_str;
			url_to_hash[url_str] = hash;
			hash_to_host[host_part] = host_str;
			host_to_hash[host_str] = host_part;
		}
	}

	return 0;
}
