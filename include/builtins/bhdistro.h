// =============================================================================
//  Dynamic Adaptive System for Hierarchical Multipole Methods (DASHMM)
//
//  Copyright (c) 2015-2017, Trustees of Indiana University,
//  All rights reserved.
//
//  This software may be modified and distributed under the terms of the BSD
//  license. See the LICENSE file for details.
//
//  This software was created at the Indiana University Center for Research in
//  Extreme Scale Technologies (CREST).
// =============================================================================


#ifndef __DASHMM_BH_DISTRO_H__
#define __DASHMM_BH_DISTRO_H__


/// \file
/// \brief Declaration of BH Distribution Policy


#include <queue>

#include "dashmm/dag.h"


namespace dashmm {


/// This distribution policy places all content on the root locality
///
/// This is intended to be the simplest possibly distribution policy. It is
/// unlikely to be a good choice, unless one is using only one locality to
/// begin with.
class BHDistro {
 public:
  BHDistro() { }

  void compute_distribution(DAG &dag);
  static void assign_for_source(DAGInfo &dag, int locality, int height) { }
  static void assign_for_target(DAGInfo &dag, int locality) { }

 private:
  std::queue<DAGNode *> collect_readies(DAG &dag);
  void compute_locality(DAGNode *node);
  void mark_upstream_nodes(DAGNode *node, std::queue<DAGNode *> &master);
  bool distribution_complete(DAG &dag);
};


} // dashmm


#endif // __DASHMM_BH_DISTRO_H__
