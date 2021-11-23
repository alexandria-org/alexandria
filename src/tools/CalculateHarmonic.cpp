
#include "CalculateHarmonic.h"

#include "config.h"
#include "link_index/Link.h"
#include "parser/URL.h"
#include "system/ThreadPool.h"
#include "algorithm/Algorithm.h"
#include "algorithm/HyperBall.h"
#include <iostream>
#include <vector>
#include <mutex>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/filesystem.hpp>
#include <unordered_map>
#include <unordered_set>

using namespace std;

namespace Tools {

	unordered_map<uint64_t, string> run_uniq_host(const vector<string> files) {

		unordered_map<uint64_t, string> hosts;

		for (const string &warc_path : files) {

			ifstream infile(warc_path);
			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			string line;
			while (getline(decompress_stream, line)) {
				const URL url(line.substr(0, line.find("\t")));
				uint64_t host_hash = url.host_hash();
				if (hosts.count(host_hash) == 0) {
					hosts[host_hash] = url.host();
				}
			}
		}

		return hosts;
	}

	struct pair_hash {
		inline size_t operator() (const pair<uint32_t, uint32_t> &p) const {
			return (uint64_t)p.first << 32 | (uint64_t)p.second;
		}
	};

	unordered_set<pair<uint32_t, uint32_t>, pair_hash> run_uniq_link(const vector<string> files, const unordered_map<uint64_t, uint32_t> &hosts) {

		unordered_set<pair<uint32_t, uint32_t>, pair_hash> edges;

		for (const string &warc_path : files) {

			ifstream infile(warc_path);
			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			string line;
			while (getline(decompress_stream, line)) {
				const Link link(line);

				const uint64_t source_hash = link.source_url().host_hash();
				const uint64_t target_hash = link.target_url().host_hash();

				const size_t source_count = hosts.count(source_hash);
				const size_t target_count = hosts.count(target_hash);
				if (source_count && target_count) {
					// Link between two hosts in the host map.
					edges.insert(make_pair(hosts.at(source_hash), hosts.at(target_hash)));
				}
			}
		}

		return edges;
	}

	void calculate_harmonic_hosts() {

		const size_t num_threads = 12;

		vector<string> files;
		for (const string &batch : Config::batches) {

			const string file_name = string("/mnt/crawl-data/") + batch + "/warc.paths.gz";

			ifstream infile(file_name);

			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			string line;
			while (getline(decompress_stream, line)) {
				string warc_path = string("/mnt/") + line;
				const size_t pos = warc_path.find(".warc.gz");
				if (pos != string::npos) {
					warc_path.replace(pos, 8, ".gz");
				}

				files.push_back(warc_path);
			}
		}

		vector<vector<string>> chunks;
		Algorithm::vector_chunk<string>(files, files.size() / (num_threads * 200), chunks);

		ThreadPool pool(num_threads);
		vector<future<unordered_map<uint64_t, string>>> results;

		for (const vector<string> &chunk : chunks) {

			results.emplace_back(pool.enqueue([chunk] {
				return run_uniq_host(chunk);
			}));
		}

		unordered_map<uint64_t, string> hosts;
		size_t idx = 0;
		cout.precision(2);
		for (auto &result : results) {
			const unordered_map<uint64_t, string> result_map = result.get();
			for (const auto &iter : result_map) {
				hosts[iter.first] = iter.second;
			}
			const double percent = (100.0*(double)idx/results.size());
			cout << "hosts contains " << hosts.size() << " elements " << percent << "% done" << endl;
			idx++;
		}

		idx = 0;
		ofstream outfile("/mnt/hosts.txt", ios::trunc);
		for (const auto &iter : hosts) {
			outfile << idx << '\t' << iter.first << '\t' << iter.second << '\n';
			idx++;
		}
		outfile.close();
	}

	unordered_map<uint64_t, uint32_t> read_hosts_file() {

		// Load the hosts from /mnt/hosts.txt
		ifstream infile("/mnt/hosts.txt");

		unordered_map<uint64_t, uint32_t> ret;

		string line;
		while (getline(infile, line)) {
			vector<string> parts;
			boost::algorithm::split(parts, line, boost::is_any_of("\t"));

			uint32_t id = stoi(parts[0]);
			uint64_t hash = stoull(parts[1]);
			ret[hash] = id;
		}

		return ret;
	}

	vector<uint32_t> read_hosts_file_vec() {

		// Load the hosts from /mnt/hosts.txt
		ifstream infile("/mnt/hosts.txt");

		vector<uint32_t> ret;

		string line;
		while (getline(infile, line)) {
			vector<string> parts;
			boost::algorithm::split(parts, line, boost::is_any_of("\t"));

			uint32_t id = stoi(parts[0]);
			ret.push_back(id);
		}

		return ret;
	}

	vector<uint32_t> *read_edge_file(size_t vlen) {

		// Load the hosts from /mnt/hosts.txt
		ifstream infile("/mnt/edges.txt");

		vector<uint32_t> *edge_map = new vector<uint32_t>[vlen];

		string line;
		while (getline(infile, line)) {
			vector<string> parts;
			boost::algorithm::split(parts, line, boost::is_any_of("\t"));

			uint32_t from = stoi(parts[0]) - 1; // I think we are counting from 0 now but from 1 when we created the edge file.
			uint32_t to = stoi(parts[1]) - 1;
			edge_map[to].push_back(from);
		}

		return edge_map;
	}

	void calculate_harmonic_links() {

		const size_t num_threads = 12;

		unordered_map<uint64_t, uint32_t> hosts = read_hosts_file();

		cout << "loaded " << hosts.size() << " hosts" << endl;

		vector<string> files;
		for (const string &batch : Config::link_batches) {

			const string file_name = string("/mnt/crawl-data/") + batch + "/warc.paths.gz";

			ifstream infile(file_name);

			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			string line;
			while (getline(decompress_stream, line)) {
				string warc_path = string("/mnt/") + line;
				const size_t pos = warc_path.find(".warc.gz");

				if (pos != string::npos) {
					warc_path.replace(pos, 8, ".links.gz");
				}

				files.push_back(warc_path);
			}
		}

		vector<vector<string>> chunks;
		Algorithm::vector_chunk<string>(files, files.size() / (num_threads * 500), chunks);

		ThreadPool pool(num_threads);
		vector<future<unordered_set<pair<uint32_t, uint32_t>, pair_hash>>> results;

		for (const vector<string> &chunk : chunks) {
			results.emplace_back(pool.enqueue([chunk, &hosts] {
				return run_uniq_link(chunk, hosts);
			}));
		}

		unordered_set<pair<uint32_t, uint32_t>, pair_hash> edges;
		size_t idx = 0;
		cout.precision(2);
		for (auto &result : results) {
			const unordered_set<pair<uint32_t, uint32_t>, pair_hash> result_set = result.get();
			size_t idasd = 0;
			for (const pair<uint32_t, uint32_t> &edge : result_set) {
				edges.insert(edge);
				idasd++;
			}
			const double percent = (100.0*(double)idx/results.size());
			cout << "edges contains " << edges.size() << " elements " << percent << "% done" << endl;
			idx++;
		}

		ofstream outfile("/mnt/edges.txt", ios::trunc);
		for (const pair<uint32_t, uint32_t> edge : edges) {
			outfile << edge.first << '\t' << edge.second << '\n';
		}
		outfile.close();
	}

	void calculate_harmonic() {

		const size_t num_threads = 8;

		vector<uint32_t> hosts = read_hosts_file_vec();
		const vector<uint32_t> *edge_map = read_edge_file(hosts.size());

		cout << "loaded " << hosts.size() << " hosts" << endl;

		cout << "running harmonic centrality algorithm on " << num_threads << " threads" << endl;

		//vector<double> harmonic = Algorithm::harmonic_centrality_threaded(hosts.size(), edge_map, 3, num_threads);

		vector<double> harmonic = Algorithm::hyper_ball(hosts.size(), edge_map);

		delete [] edge_map;

		// Save harmonic centrality.
		ofstream outfile("/mnt/harmonic.txt", ios::trunc);
		for (size_t i = 0; i < hosts.size(); i++) {
			outfile << fixed << hosts[i] << '\t' << harmonic[i] << '\n';
		}

	}

}

