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

#include "Algorithm.h"
#include <iostream>
#include <set>
#include <numeric>
#include <map>
#include <unordered_map>
#include <unordered_set>

namespace Algorithm {

	vector<int> intersection(const vector<vector<int>> &input) {

		if (input.size() == 0) return {};

		size_t shortest_vector_position = 0;
		size_t shortest_len = SIZE_MAX;
		size_t iter_index = 0;
		for (const vector<int> &vec : input) {
			if (shortest_len > vec.size()) {
				shortest_len = vec.size();
				shortest_vector_position = iter_index;
			}
			iter_index++;
		}

		vector<size_t> positions(input.size(), 0);
		vector<int> intersection;

		while (positions[shortest_vector_position] < shortest_len) {

			bool all_equal = true;
			int value = input[shortest_vector_position][positions[shortest_vector_position]];

			size_t iter_index = 0;
			for (const vector<int> &vec : input) {
				const size_t len = vec.size();

				size_t *pos = &(positions[iter_index]);
				while (*pos < len && value > vec[*pos]) {
					(*pos)++;
				}
				if (*pos < len && value < vec[*pos] || *pos >= len) {
					all_equal = false;
					break;
				}
				iter_index++;
			}
			if (all_equal) {
				intersection.push_back(input[shortest_vector_position][positions[shortest_vector_position]]);
			}

			positions[shortest_vector_position]++;
		}

		return intersection;
	}

	/*
		Returns partitions with indices that are smaller than the values in the dims vector.
		For example:
		dims = {2,2} gives {0,0}, {1,0}, {0,1}, {1,1}
		dims = {2,3} gives {0,0}, {1,0}, {0,1}, {1,1}, {0,2}, {1,2}
	*/
	vector<vector<int>> incremental_partitions(const vector<int> &dims, int limit) {
		vector<vector<int>> res;
		set<vector<int>> uniq;
		vector<int> initial(dims.size(), 0);
		res.push_back(initial);
		uniq.insert(initial);

		for (size_t j = 0; j < res.size(); j++) {
			vector<int> vec = res[j];
			for (size_t i = 0; i < vec.size(); i++) {
				if (vec[i] < dims[i]-1) {
					vector<int> copy(vec);
					copy[i]++;

					res.push_back(copy);
					uniq.insert(copy);
					if (uniq.size() >= limit) break;
				}
			}
			if (uniq.size() >= limit) break;
		}

		vector<vector<int>> ret(uniq.begin(), uniq.end());
		sort(ret.begin(), ret.end(), [](const vector<int> &a, const vector<int> &b) {
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
	vector<double> harmonic_centrality(const vector<uint64_t> vertices, const set<pair<uint64_t, uint64_t>> edges, size_t depth) {
		vector<double> harmonics;

		unordered_map<uint64_t, vector<uint64_t>> edge_map;

		for (const pair<uint64_t, uint64_t> &edge : edges) {
			/*
			second -> first mapping because we want to traverse the edges in the opposite direction of the edge. Incoming edges should increase
			harmonic centrality of vertex.
			*/
			edge_map[edge.second].push_back(edge.first);
		}

		for (const uint64_t vertex : vertices) {
			map<int, unordered_set<uint64_t>> level_sets;
			unordered_set<uint64_t> all;
			level_sets[0].insert(vertex);
			all.insert(vertex);
			double harmonic = 0.0;
			/*
				If we can assume the average number of incoming edges per vertex to be constant these loops should be O(1) in n.
				Example, if we have n = 10 000 000 vertices and 10 inbound edges on each vertex these loops should be
				(first loop is depth) X (worst case second loop is 10^depth) X (inner loop is 10)
				depth * 10^depth * 10
				independent of n
			*/
			for (int level = 1; level <= depth; level++) {
				for (const uint64_t v : level_sets[level - 1]) {
					for (const uint64_t edge : edge_map[v]) {
						if (all.count(edge) == 0) {
							level_sets[level].insert(edge); // Average O(1)
							all.insert(edge); // Average O(1)
						}
					}
				}
				harmonic += (double)level_sets[level].size() / level;
			}

			harmonics.push_back(harmonic);
		}

		return harmonics;
	}

}
