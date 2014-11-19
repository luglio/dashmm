// =============================================================================
//  DASHMM 
//
//  Draft of tree construction for DASHMM
//  dashmm-tree.h
//
//  Copyright (c) 2014, Trustees of Indiana University,
//  All rights reserved.
//
//  This software may be modified and distributed under the terms of the BSD
//  license.  See the LICENSE file for details.
//
//  This software was created at the Indiana University Center for Research in
//  Extreme Scale Technologies (CREST).
//
//  Authors:
//    Jackson DeBuhr, Indiana University <jdebuhr [at] indiana.edu>
// =============================================================================

#ifndef __DASHMM_TREE_H__
#define __DASHMM_TREE_H__


#include "hpx/hpx.h"


//TODO pick these in some kind of intelligent fashion
// These would need to be tuned based on actual performance I think.

///
/// \def TREE_CREATE_BRANCHING_FACTOR
/// 
/// The number of branches in the parallel spawn of actions over a set of
/// records in the global address space.
///
#define TREE_CREATE_BRANCHING_FACTOR 2

///
/// \def TREE_CREATE_CHUNK_SIZE
///
/// The number of records that will not be divided in the parallel spawn over
/// the records.
///
#define TREE_CREATE_CHUNK_SIZE 10


///
/// \type dashmm_volume_t
///
/// Used to represent rectangular volumes.
///
typedef struct {
  double a[3];          ///< The low corner of the rectangular prism
  double b[3];          ///< The high corner of the rectangular prism
} dashmm_volume_t;


///
/// \type dashmm_point_t
///
/// The point type will be used to represent the source or target points 
/// for DASHMM. In this incarnation, only the positions and an integer used
/// during sorting are saved.
///
/// Eventually, the moments or the forces would be added to this type.
///
typedef struct {
  double pos[3];        ///< The position of the point
  int sort;             ///< Used in sorting the points geometrically
} dashmm_point_t;


///
/// \type dashmm_tree_node_t
///
/// The nodes of the tree. Most of the members are self-explanatory. In a few
/// cases some members are used for multiple purposes. These cases are
/// documented below. In the future, likely the multiple use cases will be 
/// captured with a union.
///
/// Eventually the nodes will also need to store moment information and likely
/// other values.
/// 
typedef struct {
  dashmm_volume_t vol;  ///< the volume of the node
  
  hpx_addr_t parent;    ///< parent of this node
  hpx_addr_t child[8];  ///< children of this node
  
  hpx_addr_t points;    ///< address of the first point represented by this node
  uint32_t n_points;    ///< number of points represented by this node
  uint32_t n_arrived;   ///< number of points that have arrived
} dashmm_tree_node_t;


///
/// \type dashmm_tree_t
///
/// This type stores all the relevant information about a tree. This includes
/// the address of the topnodes, and the various parameters that define
/// the resulting tree.
typedef struct {
  dashmm_volume_t vol;  ///< the volume of the root
  int refinement;       ///< the node refinement limit
  int top_depth;        ///< the depth of the top of the tree
  hpx_addr_t topnodes;  ///< the topnodes of the tree
  int n_topnodes;       ///< the number of topnodes
} dashmm_tree_t;



//NOTE: for the time being, we prototype everything here. Eventually, we would
// only expose those routines needed from outside.


//******* user interface functions **********************************


///
/// \brief Create a tree from the given points.
///
/// The returned address will point to some global memory containing the
/// data in a dashmm_tree_t. This function will not return before the tree is
/// complete, and the data is globally visible in the returned address.
///
/// The tree itself will be in two parts, cut based on depth from the root 
/// node. The top part of the tree, down to @param top_depth, will be a 
/// complete refinement of the given volume. These `topnodes' will always
/// exist in the tree, and could be empty of points. Below those are the
/// `branches' of the tree. Associated with each of the most refined topnodes
/// is a branch of nodes that refine the contined particles to the given
/// @param refinement limit. The incoming point data is replicated and 
/// reordered in the branches of the tree. This memory replication is intended
/// to model the eventual use case that the input points data is taken from
/// a more complete user-defined record that may contain application specific
/// information about which DASHMM does not care.
///
/// However, to work with the input points, the system needs to know the 
/// distribution of those points in the global address space, hence the
/// @param n_per_block parameter.
///
/// \param points - the global address of the points from which to form the tree
/// \param n_points - the number of points
/// \param n_per_block - the number of points per GAS block in @param points
/// \param vol - the volume of the root node
/// \param refinement - the largest number of points allowed in a leaf node
/// \param top_depth - the number of levels of full refinement at the top of
///                    the tree.
/// 
/// \return - The global address of the created tree
///
hpx_addr_t dashmm_tree_create(hpx_addr_t points,
                              int n_points,
                              int n_per_block,
                              dashmm_volume_t vol,
                              int refinement,
                              int top_depth);


//******* utility functions *****************************************


///
/// \brief Register the actions needed by this file with the runtime
///
/// Currently all of the actions are registered here, but it is possible that
/// the functionality in this file might be split among other files, in which
/// case each would have such a function.
///
void dashmm_tree_register_actions(void);


///
/// \brief Return the child of the given volume in which the given point would 
///        be found.
///
/// \param vol - the volume under consideration
/// \param pos - the 3d position of the point under consideration
///
/// \return - which child (from 0-7) 
///
int dashmm_volume_which_child(dashmm_volume_t vol, double *pos);


///
/// \brief Return the volume of the given child
///
/// \param vol - a volume
/// \param which - an integer between 0 and 7
///
/// \return - the volume of the given child of the given volume
///
dashmm_volume_t dashmm_volume_of_child(dashmm_volume_t vol, int which);


///
/// \brief invoke the named action
///
/// This is included only as a means for testing the branch refinement portion
/// of the tree construction. Such a method would likely not be exposed to the
/// eventual users of DASHMM.
///
/// \param node - the node on which to invoke the action
/// \param vol - the volume to give the node
/// \param parent - the parent to give the node
/// \param points - the address of the points to associate with the node
/// \param n_points - the number of points in the given array
/// \param refine_limit - the maximum number of points per leaf. If this is
///                       non-zero, this will also cause the node to be
///                       refined. Otherwise this will merely initialize the 
///                       node.
///
void dashmm_invoke_tree_init_child_sync(hpx_addr_t node,
                                   dashmm_volume_t vol, hpx_addr_t parent,
                                   hpx_addr_t points, int n_points, 
                                   int refine_limit);
                                   

///
/// \brief Find the level of the topnode at the given index.
///
/// Topnodes are indexed linearly in levelwise order. The root node is at
/// index 0. The eight nodes of level one are the next 8 topnodes, and so on.
/// This routine takes the index, and returns which level the topnode would be
/// a member of.
///
/// \param index - the index of the topnode
///
/// \return - the level of the given index
///
int dashmm_tree_topnode_level(int index);


///
/// \brief This find the 3d index of a topnode
///
/// In each level, the topnodes are arranged in a simple fashion, with the
/// nodes being labeled first in the x, then the y, and finally the z. Though,
/// this detail is subject to change, and is not really that important for
/// the user in any event.
///
/// \param index - the linear index of the topnode
/// \param level - the level of the topnode
/// \param array - address of 3 integers in which to store the 3d index
///
void dashmm_tree_topnode_index_in_level(int index, int level, int *array);


///
/// \brief Returns the volume of the indicated topnode
///
/// \param vol - the volume of the root node
/// \param level - the level of the topnode under consideration
/// \param index - the 3d index of the topnode
///
/// \return - the volume of the given topnode
/// 
dashmm_volume_t dashmm_tree_topnode_volume(dashmm_volume_t vol, 
                                           int level, int *index);


///
/// \brief Return the global address of the indicated topnode.
///
/// \param base - the base of the global array containing the topnodes
/// \param level - the level of the topnode
/// \param index - the 3d index of the topnode
///
/// \return - the global address of the indicated node
///
hpx_addr_t dashmm_tree_topnode_address(hpx_addr_t base, int level, int *index);


///
/// \brief Return which of the topnodes at a given level inside which the 
///        specified point could be found.
///
/// At the moment, this is only used for the case when @param level is set to
/// the finest level of the topnodes, but it could be used for any of the top
/// levels.
///
/// \param vol - the volume of the root node
/// \param level - the level of the topnode 
/// \param point - the point under consideration
/// \param index - the address of three integers in which to store the 3d index
///
void dashmm_tree_topnode_from_point(dashmm_volume_t vol, int level,
                                    dashmm_point_t *point, int *index);


///
/// \brief Spawn the given action over all the records of a given global array
///
/// This will cause the given action to be invoked on each record in the given
/// array. ...
///
hpx_addr_t dashmm_parallel_spawn(
                        hpx_addr_t base_record,       //first record
                        int n_records,                //number of records
                        int64_t record_size,          //types match HPX-5
                        uint32_t block_size,          // hpx_addr_add
                        hpx_action_t terminal_action, //action to take
                        int branching_factor,         //tree branches 
                        int chunk_size,               //will not refine this
                        void *args,
                        int64_t arg_size);
                                        


//actions
int dashmm_tree_node_refine(void *args);
int dashmm_tree_points_refine(void *args);
int dashmm_tree_init_child(void *args);
int dashmm_parallel_record_spawn(void *args);
int dashmm_tree_topnode_count(void *args);
int dashmm_tree_topnode_init(void *args);
int dashmm_tree_topnode_increment_count(void *args);
int dashmm_tree_topnode_points_alloc(void *args);
int dashmm_tree_topnode_start_refine(void *args);

#endif
