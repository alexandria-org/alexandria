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

#pragma once

#include <vector>
#include <set>
#include <unordered_map>
#include <cstdint>

namespace Algorithm {

	template<class T>
	void vector_chunk(const std::vector<T> &vec, size_t chunk_size, std::vector<std::vector<T>> &dest) {
		std::vector<T> chunk;
		for (T item : vec) {
			chunk.push_back(item);
			if (chunk.size() == chunk_size) {
				dest.push_back(chunk);
				chunk.clear();
			}
		}
		if (chunk.size()) {
			dest.push_back(chunk);
		}
	}

	std::vector<std::vector<int>> incremental_partitions(const std::vector<int> &dims, size_t limit);

	/*
		Calculates the harmonic centrality for vertices and edges. The returning vector has the harmonic centrality for vertex i at position i.
		The depth parameter is the maximum level to traverse in the neighbour tree.
		The edges set contains pairs of edges (from vertex, to vertex)
	*/
	std::vector<double> harmonic_centrality(size_t vlen, const std::set<std::pair<uint32_t, uint32_t>> &edges, size_t depth);
	std::vector<double> harmonic_centrality(size_t vlen, const std::vector<uint32_t> *edge_map, size_t depth);
	std::vector<double> harmonic_centrality_threaded(size_t vlen, const std::set<std::pair<uint32_t, uint32_t>> &edges, size_t depth,
			size_t num_threads);
	std::vector<double> harmonic_centrality_threaded(size_t vlen, const std::vector<uint32_t> *edge_map,
			size_t depth, size_t num_threads);

	std::vector<uint32_t> *set_to_edge_map(size_t n, const std::set<std::pair<uint32_t, uint32_t>> &edges);
}
