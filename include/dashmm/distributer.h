#ifndef __DASHMM_DISTRIBUTER_H__
#define __DASHMM_DISTRIBUTER_H__
#include <vector>
#include <set>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <float.h>
#include <math.h>
#include <assert.h> 

// Cell holds information on how it is partitioned and how it relates to 
// its spacial neighbors and the other cells in its partition 
struct Cell{
	int x;
	int y;	
	int z;
	int nparticles;
	int available_neighbors;
	int group_neighbors;
	double structure_priority;
	double neighborness;
	double priority;
	bool is_free;
	int index;

	Cell() : index(-1), is_free(true) {}
};

class Distributer{
public:
	Distributer() {}

	// counts comes in simple order x+d*y+d^2*z
	void setup(int nparts, int* counts, int len) {
		length = (int)cbrt(len) + 0.5; // round up for safety
		nparticles = 0;
		npartitions = nparts;
		std::vector<std::vector<std::vector<Cell*> > > g;
		for(int x=0;x<length;++x){
			std::vector<std::vector<Cell*> > base;
			for(int y=0;y<length;++y){
				std::vector<Cell*> col;
				for(int z=0;z<length;++z){
					Cell* cell = new Cell;
					cell->x = x; 
					cell->y = y;
					cell->z = z;
					uint64_t key = simple_key(x, y, z, length);
					cell->nparticles = counts[key];
					nparticles += counts[key];
					col.push_back(cell);	
				}
				base.push_back(col);
			}
			g.push_back(base);
		}
		grid = g;
	}

	// TODO: Add functionality to let user change between partitioning algorithms
	int* partition() {
		// partition with selected algorithm
		partition_hilbert();
		// get respective partition indecies and return in simple order
		int* retval = rank_indecies();

	
		// print indecies here
		std::cout<<"Indecies:"<<std::endl;
		for(int i=0;i<length*length*length;++i){
			std::cout<<retval[i]<<" ";
		}
		std::cout<<std::endl;

		return retval;
	}


private:
	// REPRESENTATION:
	int length; 
	int nparticles;
	int npartitions;
	std::vector<std::vector<std::vector<Cell*> > > grid;
	std::vector<std::vector<Cell*> > partitions;


	// PARTITIONING ALGORITHMS:

	void partition_morton() {
		int len = length*length*length;
		// create array holding the cells in morton order 
		Cell* cells_by_morton[len];
		for(unsigned x=0;x<length;++x){
			for(unsigned y=0;y<length;++y){
				for(unsigned z=0;z<length;++z){
					uint64_t key = morton_key(x, y, z);
					cells_by_morton[key] = grid[x][y][z];
				}
			}
		}

		// Get the partitioning cuts
		int* cumulative = new int[len + 1];
		cumulative[0] = 0;
		for (int i = 1; i <= len; ++i) {
			cumulative[i] = cells_by_morton[i-1]->nparticles + cumulative[i - 1];
		}
	
		int target_share = cumulative[len] / npartitions;

		int *cuts = new int[npartitions]();
		for (int split = 1; split < npartitions; ++split) {
		  int my_target = target_share * split;
		  auto fcn = [&my_target] (const int &a) -> bool {return a < my_target;};
		  int *splitter = std::partition_point(cumulative, &cumulative[len + 1],
		                                       fcn);
		  int upper_delta = *splitter - my_target;
		  assert(upper_delta >= 0);
		  int lower_delta = my_target - *(splitter - 1);
		  assert(lower_delta >= 0);
		  int split_index = upper_delta < lower_delta
		                      ? splitter - cumulative
		                      : (splitter - cumulative) - 1;
		  --split_index;
		  assert(split_index >= 0);

		  cuts[split - 1] = split_index;
		}
		cuts[npartitions - 1] = len - 1;

		// use the cuts to partition the grid
		for(int i=0;i<npartitions;++i) {
			std::vector<Cell*> group;
			int start = i > 0 ? cuts[i-1]+1 : 0;
			for(int j=start;j<=cuts[i];++j){
				add_to_partition(cells_by_morton[j], group, i);

			}
			partitions.push_back(group);
		}
	}
	
	void partition_hilbert() {
		int len = length*length*length;
		// create array holding the cells in hilbert order 
		Cell* cells_by_hilbert[len];
		for(unsigned x=0;x<length;++x){
			for(unsigned y=0;y<length;++y){
				for(unsigned z=0;z<length;++z){
					uint64_t key = hilbert_key(x, y, z, length);
					cells_by_hilbert[key] = grid[x][y][z];
				}
			}
		}

		// Get the partitioning cuts
		int* cumulative = new int[len + 1];
		cumulative[0] = 0;
		for (int i = 1; i <= len; ++i) {
			cumulative[i] = cells_by_hilbert[i-1]->nparticles + cumulative[i - 1];
		}
	
		int target_share = cumulative[len] / npartitions;

		int *cuts = new int[npartitions]();
		for (int split = 1; split < npartitions; ++split) {
		  int my_target = target_share * split;
		  auto fcn = [&my_target] (const int &a) -> bool {return a < my_target;};
		  int *splitter = std::partition_point(cumulative, &cumulative[len + 1],
		                                       fcn);
		  int upper_delta = *splitter - my_target;
		  assert(upper_delta >= 0);
		  int lower_delta = my_target - *(splitter - 1);
		  assert(lower_delta >= 0);
		  int split_index = upper_delta < lower_delta
		                      ? splitter - cumulative
		                      : (splitter - cumulative) - 1;
		  --split_index;
		  assert(split_index >= 0);

		  cuts[split - 1] = split_index;
		}
		cuts[npartitions - 1] = len - 1;

		// use the cuts to partition the grid
		for(int i=0;i<npartitions;++i) {
			std::vector<Cell*> group;
			int start = i > 0 ? cuts[i-1]+1 : 0;
			for(int j=start;j<=cuts[i];++j){
				add_to_partition(cells_by_hilbert[j], group, i);

			}
			partitions.push_back(group);
		}
	}

	int* partition_ADAPT();

	// return an array in simple order with the rank indecies
	int* rank_indecies() {
		int* retval = new int[length*length*length];
		for(int x=0;x<length;++x){
			for(int y=0;y<length;++y){
				for(int z=0;z<length;++z){
					uint64_t key = simple_key(x, y, z, length); // TODO: see if fixed (changed to hilbert order)
					int index = grid[x][y][z]->index;
					assert(index >= 0);
					retval[key] = index;
				}
			}
		}
		return retval;
	}

	void add_to_partition(Cell* cell, std::vector<Cell*>& group, int index) {
		cell->is_free = false;
		cell->index = index;
		group.push_back(cell);
	}

	// SPACE FILLING CURVES:

	// Split the bits of an integer to be used in a Morton Key
	static uint64_t split(unsigned k) {
		uint64_t split = k & 0x1fffff;
		split = (split | split << 32) & 0x1f00000000ffff;
		split = (split | split << 16) & 0x1f0000ff0000ff;
		split = (split | split << 8)  & 0x100f00f00f00f00f;
		split = (split | split << 4)  & 0x10c30c30c30c30c3;
		split = (split | split << 2)  & 0x1249249249249249;
		return split;
	}

	// Compute the Morton key for a gives set of indices
	static uint64_t morton_key(unsigned x, unsigned y, unsigned z) {
		uint64_t key = 0;
		key |= split(x) | split(y) << 1 | split(z) << 2;
		return key;
	}

	static uint64_t simple_key(unsigned x, unsigned y, unsigned z, unsigned max) {
		return x + y * max + z * max * max;
	}

	// Group transformations for hilbert subcubes
	int h_indecies[8] {0, 1, 3, 2, 7, 6, 4, 5};
	int h_transforms[8][8] {{0, 7, 4, 3, 2, 5, 6, 1},
													{0, 3, 4, 7, 6, 5, 2, 1},
													{0, 3, 4, 7, 6, 5, 2, 1}, 
													{2, 1, 0, 3, 4, 7, 6, 5},
													{2, 1, 0, 3, 4, 7, 6, 5},
													{6, 5, 2, 1, 0, 3, 4, 7},
													{6, 5, 2, 1, 0, 3, 4, 7},
													{6, 1, 2, 5, 4, 3, 0, 7}};

	// Compute the Hilbert key for a given set of indecies
	int hilbert_key(unsigned x, unsigned y, unsigned z, unsigned slen) {
		// TODO: redo with octal bit magic
		int* offsets = new int[8]{0, 1, 2, 3, 4, 5, 6, 7};
		int* offsets_next = new int[8];
		int key = 0;

		for(int s=slen;s>1;s/=2){
			// get place in larger hilbert order from position and offsets
			int m_index = 4*(z%s >= s/2) + 2*(y%s >= s/2)	+ (x%s >= s/2);		
			// update key
			key += (s*s*s)/8 * offsets[h_indecies[m_index]];
			// update offsets 
			for(int i=0;i<8;++i){
				// TODO: Explain this line of code with group transformations
				offsets_next[i] = h_transforms[offsets[h_indecies[m_index]]][offsets[i]];
			}
			int* tmp;
			tmp = offsets_next;
			offsets_next = offsets;
			offsets = tmp;
		}
		delete [] offsets;
		delete [] offsets_next;

		return key;
	}

};



/*
// returns the (x,y,z) coords of a point in simple order
unsigned* simple_xyz(static uint64_t key, unsigned max) {
	unsigned x = key % max;
	key = (key - x) / max;
	unsigned y = key % max;
	unsigned z = (key - y) / max;

	unsigned* xyz = new usigned[3]{x, y, z};
	return xyz;
}*/
















#endif
