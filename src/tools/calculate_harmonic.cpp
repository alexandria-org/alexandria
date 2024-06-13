
#include "calculate_harmonic.h"
#include "splitter.h"

#include "config.h"
#include "url_link/link.h"
#include "URL.h"
#include "common/ThreadPool.h"
#include "algorithm/algorithm.h"
#include "algorithm/hyper_ball.h"
#include <iostream>
#include <vector>
#include <mutex>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <unordered_map>
#include <unordered_set>

namespace tools {

	std::unordered_map<uint64_t, std::string> run_uniq_host(const std::vector<std::string> files) {

		std::unordered_map<uint64_t, std::string> hosts;

		for (const std::string &warc_path : files) {

			std::ifstream infile(warc_path);
			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			std::string line;
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
		inline size_t operator() (const std::pair<uint32_t, uint32_t> &p) const {
			return (uint64_t)p.first << 32 | (uint64_t)p.second;
		}
	};

	std::unordered_set<std::pair<uint32_t, uint32_t>, pair_hash> run_uniq_link(const std::vector<std::string> files, const std::unordered_map<uint64_t, uint32_t> &hosts) {

		std::unordered_set<std::pair<uint32_t, uint32_t>, pair_hash> edges;

		for (const std::string &warc_path : files) {

			std::ifstream infile(warc_path);
			boost::iostreams::filtering_istream decompress_stream;
			decompress_stream.push(boost::iostreams::gzip_decompressor());
			decompress_stream.push(infile);

			std::string line;
			while (getline(decompress_stream, line)) {
				const url_link::link link(line);

				const uint64_t source_hash = link.source_url().host_hash();
				const uint64_t target_hash = link.target_url().host_hash();

				const size_t source_count = hosts.count(source_hash);
				const size_t target_count = hosts.count(target_hash);
				if (source_count && target_count) {
					// Link between two hosts in the host map.
					edges.insert(std::make_pair(hosts.at(source_hash), hosts.at(target_hash)));
				}
			}
		}

		return edges;
	}

	void calculate_harmonic_hosts() {

		auto files = generate_list_with_target_url_files();

		std::vector<std::vector<std::string>> chunks;
		algorithm::vector_chunk<std::string>(files, files.size() / (s_num_threads * 200), chunks);

		ThreadPool pool(s_num_threads);
		std::vector<std::future<std::unordered_map<uint64_t, std::string>>> results;

		for (const std::vector<std::string> &chunk : chunks) {

			results.emplace_back(pool.enqueue([chunk] {
				return run_uniq_host(chunk);
			}));
		}

		std::unordered_map<uint64_t, std::string> hosts;
		size_t idx = 0;
		std::cout.precision(2);
		for (auto &result : results) {
			const std::unordered_map<uint64_t, std::string> result_map = result.get();
			for (const auto &iter : result_map) {
				hosts[iter.first] = iter.second;
			}
			const double percent = (100.0*(double)idx/results.size());
			std::cout << "hosts contains " << hosts.size() << " elements " << percent << "% done" << std::endl;
			idx++;
		}

		idx = 0;
		std::ofstream outfile(config::data_path() + "/hosts.txt", std::ios::trunc);
		for (const auto &iter : hosts) {
			outfile << idx << '\t' << iter.first << '\t' << iter.second << '\n';
			idx++;
		}
		outfile.close();
	}

	std::unordered_map<uint64_t, uint32_t> read_hosts_file() {

		// Load the hosts
		std::ifstream infile(config::data_path() + "/hosts.txt");

		std::unordered_map<uint64_t, uint32_t> ret;

		std::string line;
		while (getline(infile, line)) {
			std::vector<std::string> parts;
			boost::algorithm::split(parts, line, boost::is_any_of("\t"));

			uint32_t id = std::stoi(parts[0]);
			uint64_t hash = std::stoull(parts[1]);
			ret[hash] = id;
		}

		return ret;
	}

	std::vector<uint32_t> read_hosts_file_vec() {

		// Load the hosts
		std::ifstream infile(config::data_path() + "/hosts.txt");

		std::vector<uint32_t> ret;

		std::string line;
		while (getline(infile, line)) {
			std::vector<std::string> parts;
			boost::algorithm::split(parts, line, boost::is_any_of("\t"));

			uint32_t id = std::stoi(parts[0]);
			ret.push_back(id);
		}

		return ret;
	}

	std::unique_ptr<std::vector<uint32_t>[]> read_edge_file(size_t vlen) {

		// Load the hosts
		std::ifstream infile(config::data_path() + "/edges.txt");

		auto edge_map = std::make_unique<std::vector<uint32_t>[]>(vlen);

		std::string line;
		while (getline(infile, line)) {
			std::vector<std::string> parts;
			boost::algorithm::split(parts, line, boost::is_any_of("\t"));

			uint32_t from = std::stoi(parts[0]); // I think we are counting from 0 now but from 1 when we created the edge file.
			uint32_t to = std::stoi(parts[1]);
			edge_map[to].push_back(from);
		}

		return edge_map;
	}

	void calculate_harmonic_links() {

		std::unordered_map<uint64_t, uint32_t> hosts = read_hosts_file();

		std::cout << "loaded " << hosts.size() << " hosts" << std::endl;

		auto files = generate_list_with_target_link_files();

		std::vector<std::vector<std::string>> chunks;
		algorithm::vector_chunk<std::string>(files, files.size() / (s_num_threads * 500), chunks);

		ThreadPool pool(s_num_threads);
		std::vector<std::future<std::unordered_set<std::pair<uint32_t, uint32_t>, pair_hash>>> results;

		for (const std::vector<std::string> &chunk : chunks) {
			results.emplace_back(pool.enqueue([chunk, &hosts] {
				return run_uniq_link(chunk, hosts);
			}));
		}

		std::unordered_set<std::pair<uint32_t, uint32_t>, pair_hash> edges;
		size_t idx = 0;
		std::cout.precision(2);
		for (auto &result : results) {
			const std::unordered_set<std::pair<uint32_t, uint32_t>, pair_hash> result_set = result.get();
			size_t idasd = 0;
			for (const std::pair<uint32_t, uint32_t> &edge : result_set) {
				edges.insert(edge);
				idasd++;
			}
			const double percent = (100.0*(double)idx/results.size());
			std::cout << "edges contains " << edges.size() << " elements " << percent << "% done" << std::endl;
			idx++;
		}

		std::ofstream outfile(config::data_path() + "/edges.txt", std::ios::trunc);
		for (const std::pair<uint32_t, uint32_t>& edge : edges) {
			outfile << edge.first << '\t' << edge.second << '\n';
		}
		outfile.close();
	}

	void calculate_harmonic() {

		std::vector<uint32_t> hosts = read_hosts_file_vec();
		auto edge_map = read_edge_file(hosts.size());

		std::cout << "loaded " << hosts.size() << " hosts" << std::endl;

		std::cout << "running harmonic centrality algorithm on " << s_num_threads << " threads" << std::endl;

		//vector<double> harmonic = algorithm::harmonic_centrality_threaded(hosts.size(), edge_map, 3, num_threads);

		std::vector<double> harmonic = algorithm::hyper_ball(hosts.size(), edge_map);

		edge_map.reset(nullptr);

		// Save harmonic centrality.
		std::ofstream outfile(config::data_path() + "/harmonic.txt", std::ios::trunc);
		for (size_t i = 0; i < hosts.size(); i++) {
			outfile << std::fixed << hosts[i] << '\t' << harmonic[i] << '\n';
		}

	}

}

