/*
 * MIT License
 *
 * Alexandria.org
 *
 * Copyright (c) 2021 Josef Cullhed, <info@alexandria.org>, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "algorithm.h"
#include "profiler/profiler.h"
#include <iostream>
#include <set>
#include <numeric>
#include <map>
#include <math.h>
#include <cassert>
#include <future>
#include <cstring>

namespace algorithm {

	/*
		Returns partitions with indices that are smaller than the values in the dims vector.
		For example:
		dims = {2,2} gives {0,0}, {1,0}, {0,1}, {1,1}
		dims = {2,3} gives {0,0}, {1,0}, {0,1}, {1,1}, {0,2}, {1,2}
	*/
	std::vector<std::vector<int>> incremental_partitions(const std::vector<int> &dims, size_t limit) {
		std::vector<std::vector<int>> res;
		std::set<std::vector<int>> uniq;
		std::vector<int> initial(dims.size(), 0);
		res.push_back(initial);
		uniq.insert(initial);

		for (size_t j = 0; j < res.size(); j++) {
			std::vector<int> vec = res[j];
			for (size_t i = 0; i < vec.size(); i++) {
				if (vec[i] < dims[i]-1) {
					std::vector<int> copy(vec);
					copy[i]++;

					res.push_back(copy);
					uniq.insert(copy);
					if (uniq.size() >= limit) break;
				}
			}
			if (uniq.size() >= limit) break;
		}

		std::vector<std::vector<int>> ret(uniq.begin(), uniq.end());
		sort(ret.begin(), ret.end(), [](const std::vector<int> &a, const std::vector<int> &b) {
			int sum1 = accumulate(a.begin(), a.end(), 0);
			int sum2 = accumulate(b.begin(), b.end(), 0);
			if (sum1 == sum2) {
				int max1 = *max_element(a.begin(), a.end());
				int max2 = *max_element(b.begin(), b.end());
				if (max1 == max2) {
					return b < a;
				}
				return max1 < max2;
			}
			return sum1 < sum2;
		});
		return ret;
	}

	/*
		Calculates the harmonic centrality for vertices and edges. The returning vector has the harmonic centrality for vertex i at position i.
		The depth parameter is the maximum level to traverse in the neighbour tree.
		The edges set contains pairs of edges (from vertex, to vertex)
	*/

	/*
	 * This is the inner outer loop for calculating harmonic centrality.
	 * */
	std::vector<double> harmonic_centrality_subvector(size_t vlen, const std::vector<uint32_t> *edge_map,
			size_t depth, size_t start, size_t len) {

		char *all = new char[vlen];
		uint32_t *level1 = new uint32_t[vlen];
		uint32_t *level2 = new uint32_t[vlen];

		uint32_t *levels[2] = {level1, level2};
		size_t level_len[2] = {0, 0};

		std::vector<double> harmonics;

		profiler::instance prof("Timetaker");
		for (size_t i = start; i < start + len; i++) {
			const uint32_t vertex = i;

			level_len[0] = 0;
			level_len[1] = 0;
			memset(all, 0, vlen);

			levels[0][0] = vertex;
			level_len[0]++;
			all[vertex] = 1;

			double harmonic = 0.0;
			/*
				If we can assume the average number of incoming edges per vertex to be constant these loops should be O(1) in n.
				Example, if we have n = 10 000 000 vertices and 10 inbound edges on each vertex these loops should be
				(first loop is depth) X (worst case second loop is 10^depth) X (inner loop is 10)
				depth * 10^depth * 10
				independent of n
			*/
			size_t last_level = 0;
			size_t cur_level = 1;
			for (size_t level = 1; level <= depth; level++) {
				//for (const uint32_t &v : level[level - 1]) {
				for (size_t j = 0; j < level_len[last_level]; j++) {
					const uint32_t v = levels[last_level][j];
					for (const uint32_t &edge : edge_map[v]) {
						if (!all[edge]) {
							levels[cur_level][level_len[cur_level]++] = edge;
							all[edge] = 1;
						}
					}
				}
				if (level_len[cur_level] == 0) break;
				harmonic += (double)level_len[cur_level] / level;
				// Swap levels
				level_len[last_level] = 0;
				size_t tmp = last_level;
				last_level = cur_level;
				cur_level = tmp;
			}

			harmonics.push_back(harmonic);
		}

		delete [] level2;
		delete [] level1;
		delete [] all;

		return harmonics;
	}

	std::vector<double> harmonic_centrality(size_t vlen, const std::set<std::pair<uint32_t, uint32_t>> &edges, size_t depth) {
		std::vector<double> harmonics;

		std::vector<uint32_t> *edge_map = new std::vector<uint32_t>[vlen];
		for (const auto &edge : edges) {
			/*
			second -> first mapping because we want to traverse the edges in the opposite direction of the edge. Incoming edges should increase
			harmonic centrality of vertex.
			*/
			edge_map[edge.second].push_back(edge.first);
		}

		std::vector<double> ret = harmonic_centrality(vlen, edge_map, depth);

		delete [] edge_map;

		return ret;
	}

	std::vector<double> harmonic_centrality(size_t vlen, const std::vector<uint32_t> *edge_map, size_t depth) {
		return harmonic_centrality_subvector(vlen, edge_map, depth, 0, vlen);
	}

	std::vector<double> harmonic_centrality_threaded(size_t vlen, const std::set<std::pair<uint32_t, uint32_t>> &edges, size_t depth,
			size_t num_threads) {

		std::vector<uint32_t> *edge_map = new std::vector<uint32_t>[vlen];
		for (const auto &edge : edges) {
			/*
			second -> first mapping because we want to traverse the edges in the opposite direction of the edge. Incoming edges should increase
			harmonic centrality of vertex.
			*/
			edge_map[edge.second].push_back(edge.first);
		}

		std::vector<double> ret = harmonic_centrality_threaded(vlen, edge_map, depth, num_threads);

		delete [] edge_map;

		return ret;
	}

	std::vector<double> harmonic_centrality_threaded(size_t vlen, const std::vector<uint32_t> *edge_map, size_t depth, size_t num_threads) {

		assert(vlen >= num_threads);

		std::vector<std::future<std::vector<double>>> threads;

		// Split the vertices into several vectors.
		const size_t max_len = ceil((double)vlen / num_threads);
		for (size_t i = 0; i < vlen; i += max_len) {
			const size_t len = std::min(max_len, vlen - i);
			threads.emplace_back(std::async(std::launch::async, harmonic_centrality_subvector, vlen, edge_map, depth, i, len));
		}

		std::vector<double> harmonic;
		for (auto &thread : threads) {
			std::vector<double> part = thread.get();
			harmonic.insert(harmonic.end(), part.begin(), part.end());
		}

		return harmonic;
	}

	std::vector<uint32_t> *set_to_edge_map(size_t n, const std::set<std::pair<uint32_t, uint32_t>> &edges) {
		std::vector<uint32_t> *edge_map = new std::vector<uint32_t>[n];
		for (const auto &edge : edges) {
			/*
			second -> first mapping because we want to traverse the edges in the opposite direction of the edge. Incoming edges should increase
			harmonic centrality of vertex.
			*/
			edge_map[edge.second].push_back(edge.first);
		}

		return edge_map;
	}

}
