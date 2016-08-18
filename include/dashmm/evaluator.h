// =============================================================================
//  Dynamic Adaptive System for Hierarchical Multipole Methods (DASHMM)
//
//  Copyright (c) 2015-2016, Trustees of Indiana University,
//  All rights reserved.
//
//  This software may be modified and distributed under the terms of the BSD
//  license. See the LICENSE file for details.
//
//  This software was created at the Indiana University Center for Research in
//  Extreme Scale Technologies (CREST).
// =============================================================================


#ifndef __DASHMM_EVALUATOR_H__
#define __DASHMM_EVALUATOR_H__


/// \file include/dashmm/evaluator.h
/// \brief Definition of DASHMM Evaluator object


#include <hpx/hpx.h>

#include "dashmm/array.h"
#include "dashmm/arrayref.h"
#include "dashmm/defaultpolicy.h"
#include "dashmm/domaingeometry.h"
#include "dashmm/expansionlco.h"
#include "dashmm/point.h"
#include "dashmm/targetlco.h"
#include "dashmm/tree.h"


namespace dashmm {


/// Evaluator object
///
/// This object is a central object in DASHMM evaluations. This object bears
/// two responsibilities: performing multipole method evaluations, and
/// registration of actions with the runtime system.
///
/// The interface for the object contains only one member, evaluate(), which
/// performs a multipole method evaluation.
///
/// In addition, the object's constructor performs its second duty. DASHMM is
/// a templated library. HPX-5 requires the address of functions that are to
/// be called as actions. Thus, we must somehow manage to register the specific
/// instances of the actions DASHMM requires once the template arguments are
/// specified. By creating an Evaluator object, which requires filling out the
/// template arguments, this object is able to instantiate all the templates
/// needed by the particular evaluation represented by this object.
///
/// A further note on the use of Evaluator. To register a particular set of
/// types with the runtime, an instance of this object must be created before
/// dashmm::init(). Evaluators created after dashmm::init() will cause a
/// fatal error.
///
/// Finally, only one instance of an evaluator for a given set of types
/// should be created. Multiple instances of a particular instance of an
/// Evaluator will cause a fatal error.
///
/// In the future, this restriction may be enforced by the library.
///
/// This is a template class over the Source, Targer, Expansion and Method
/// types. For a description of the requirements of these types, please see
/// the documentation.
template <typename Source, typename Target,
          template <typename, typename> class Expansion,
          template <typename, typename,
                    template <typename, typename> class,
                    typename> class Method,
          typename DistroPolicy = DefaultDistributionPolicy>
class Evaluator {
 public:
  using source_t = Source;
  using target_t = Target;
  using expansion_t = Expansion<Source, Target>;
  using method_t = Method<Source, Target, Expansion, DistroPolicy>;
  using sourceref_t = ArrayRef<Source>;
  using targetref_t = ArrayRef<Target>;
  using targetlco_t = TargetLCO<Source, Target, Expansion, Method,
                                DistroPolicy>;
  using expansionlco_t = ExpansionLCO<Source, Target, Expansion, Method,
                                      DistroPolicy>;
  using sourcenode_t = TreeNode<Source, Target, Source, Expansion, Method,
                                DistroPolicy>;
  using targetnode_t = TreeNode<Source, Target, Target, Expansion, Method,
                                DistroPolicy>;
  using tree_t = Tree<Source, Target, Expansion, Method, DistroPolicy>;
  using distropolicy_t = DistroPolicy;

  /// The constuctor takes care of all action registration that DASHMM needs
  /// for one particular combination of Source, Target, Expansion and Method.
  ///
  /// For this to work, the related classes must mark Evaluator as a friend.
  Evaluator() {
    // TargetLCO related
    HPX_REGISTER_ACTION(HPX_FUNCTION, HPX_ATTR_NONE,
                        targetlco_t::init_, targetlco_t::init_handler,
                        HPX_POINTER, HPX_SIZE_T, HPX_POINTER, HPX_SIZE_T);
    HPX_REGISTER_ACTION(HPX_FUNCTION, HPX_ATTR_NONE,
                        targetlco_t::operation_,
                        targetlco_t::operation_handler,
                        HPX_POINTER, HPX_POINTER, HPX_SIZE_T);
    HPX_REGISTER_ACTION(HPX_FUNCTION, HPX_ATTR_NONE,
                        targetlco_t::predicate_,
                        targetlco_t::predicate_handler,
                        HPX_POINTER, HPX_SIZE_T);
    HPX_REGISTER_ACTION(HPX_DEFAULT, HPX_MARSHALLED,
                        targetlco_t::create_, targetlco_t::create_at_locality,
                        HPX_POINTER, HPX_SIZE_T);

    // ExpansionLCO related
    HPX_REGISTER_ACTION(HPX_FUNCTION, HPX_ATTR_NONE,
                        expansionlco_t::init_, expansionlco_t::init_handler,
                        HPX_POINTER, HPX_SIZE_T, HPX_POINTER, HPX_SIZE_T);
    HPX_REGISTER_ACTION(HPX_FUNCTION, HPX_ATTR_NONE,
                        expansionlco_t::operation_,
                        expansionlco_t::operation_handler,
                        HPX_POINTER, HPX_POINTER, HPX_SIZE_T);
    HPX_REGISTER_ACTION(HPX_FUNCTION, HPX_ATTR_NONE,
                        expansionlco_t::predicate_,
                        expansionlco_t::predicate_handler,
                        HPX_POINTER, HPX_SIZE_T);
    HPX_REGISTER_ACTION(HPX_DEFAULT, HPX_ATTR_NONE,
                        expansionlco_t::spawn_out_edges_,
                        expansionlco_t::spawn_out_edges_handler,
                        HPX_INT);
    HPX_REGISTER_ACTION(HPX_DEFAULT, HPX_MARSHALLED,
                        expansionlco_t::spawn_out_edges_from_remote_,
                        expansionlco_t::spawn_out_edges_from_remote_handler,
                        HPX_POINTER, HPX_SIZE_T);
    HPX_REGISTER_ACTION(HPX_DEFAULT, HPX_MARSHALLED,
                        expansionlco_t::create_from_expansion_,
                        expansionlco_t::create_from_expansion_handler,
                        HPX_POINTER, HPX_SIZE_T);

    // Tree related
    HPX_REGISTER_ACTION(HPX_DEFAULT, HPX_MARSHALLED,
                        tree_t::source_partition_,
                        tree_t::source_partition_handler,
                        HPX_POINTER, HPX_SIZE_T);
    HPX_REGISTER_ACTION(HPX_DEFAULT, HPX_MARSHALLED,
                        tree_t::target_partition_,
                        tree_t::target_partition_handler,
                        HPX_POINTER, HPX_SIZE_T);
    HPX_REGISTER_ACTION(HPX_DEFAULT, HPX_ATTR_NONE,
                        tree_t::source_apply_method_child_done_,
                        tree_t::source_apply_method_child_done_handler,
                        HPX_POINTER, HPX_POINTER, HPX_ADDR);
    HPX_REGISTER_ACTION(HPX_DEFAULT, HPX_ATTR_NONE,
                        tree_t::source_apply_method_,
                        tree_t::source_apply_method_handler,
                        HPX_POINTER, HPX_POINTER, HPX_ADDR);
    HPX_REGISTER_ACTION(HPX_DEFAULT, HPX_ATTR_NONE,
                        tree_t::target_apply_method_,
                        tree_t::target_apply_method_handler,
                        HPX_POINTER, HPX_POINTER, HPX_POINTER,
                        HPX_INT, HPX_ADDR);
    HPX_REGISTER_ACTION(HPX_DEFAULT, HPX_ATTR_NONE,
                        tree_t::source_bounds_, tree_t::source_bounds_handler,
                        HPX_ADDR, HPX_SIZE_T);
    HPX_REGISTER_ACTION(HPX_DEFAULT, HPX_ATTR_NONE,
                        tree_t::target_bounds_, tree_t::target_bounds_handler,
                        HPX_ADDR, HPX_SIZE_T);
    HPX_REGISTER_ACTION(HPX_DEFAULT, HPX_ATTR_NONE,
                        tree_t::create_S_expansions_from_DAG_,
                        tree_t::create_S_expansions_from_DAG_handler,
                        HPX_ADDR, HPX_POINTER, HPX_POINTER);
    HPX_REGISTER_ACTION(HPX_DEFAULT, HPX_ATTR_NONE,
                        tree_t::create_T_expansions_from_DAG_,
                        tree_t::create_T_expansions_from_DAG_handler,
                        HPX_ADDR, HPX_POINTER, HPX_POINTER);
    HPX_REGISTER_ACTION(HPX_DEFAULT, HPX_ATTR_NONE,
                        tree_t::edge_lists_,
                        tree_t::edge_lists_handler,
                        HPX_POINTER, HPX_SIZE_T, HPX_POINTER, HPX_SIZE_T);
    HPX_REGISTER_ACTION(HPX_DEFAULT, HPX_ATTR_NONE,
                        tree_t::instigate_dag_eval_,
                        tree_t::instigate_dag_eval_handler,
                        HPX_POINTER, HPX_POINTER);
    HPX_REGISTER_ACTION(HPX_DEFAULT, HPX_MARSHALLED,
                        tree_t::instigate_dag_eval_remote_,
                        tree_t::instigate_dag_eval_remote_handler,
                        HPX_POINTER, HPX_SIZE_T);
    HPX_REGISTER_ACTION(HPX_DEFAULT, HPX_ATTR_NONE,
                        tree_t::termination_detection_,
                        tree_t::termination_detection_handler,
                        HPX_ADDR, HPX_POINTER, HPX_SIZE_T, HPX_POINTER,
                        HPX_SIZE_T, HPX_POINTER, HPX_SIZE_T);
    HPX_REGISTER_ACTION(HPX_DEFAULT, HPX_ATTR_NONE,
                        tree_t::destroy_DAG_LCOs_,
                        tree_t::destroy_DAG_LCOs_handler,
                        HPX_POINTER, HPX_SIZE_T);

    // Actions for the evaluation
    HPX_REGISTER_ACTION(HPX_DEFAULT, HPX_MARSHALLED,
                        evaluate_, evaluate_handler,
                        HPX_POINTER, HPX_SIZE_T);

  }

  /// Perform a multipole moment evaluation
  ///
  /// Given source, targets, a refinement_limit, a method and an expansion,
  /// this routine performs a multipole method evaluation. The source and
  /// target arrays will likely be sorted by DASHMM during evaluation, so it
  /// is imperative that users include some sort of identifying data in the
  /// given arrays.
  ///
  /// In addition to sorting the data, the records in the targets Array will
  /// be given the results of the evaluation. Other than these two changes,
  /// the arrays will be unchanged. NOTE: the sources and targets are passed
  /// as const even though the data they represent will be modified. Array
  /// objects are references to data in the global address space. It is the
  /// global data that will be modified, not where that data can be found. For
  /// this reason, we can pass these arguments as const.
  ///
  /// The refinement limit controls the partitioning of the domain. A given
  /// portion of the domain will be subdivided if there are more than the
  /// given number of sources or targets in that region.
  ///
  /// The provided method and expansion are used as prototypes for any other
  /// copies of those objects that are required. Some methods have data
  /// associated with them. Some expansions require an accuracy parameter to
  /// fully specify the expansion, and so this argument will supply that
  /// accuracy.
  ///
  /// \param sources - a DASHMM Array of the source points
  /// \param targets - a DASHMM Array of the target points
  /// \param refinement_limint - the domain refinement limit
  /// \param method - a prototype of the method to use.
  /// \param n_digits - the number of digits of accuracy required
  /// \param kernelparams - the parameters needed by the kernel
  /// \param distro - an instance of the distribution policy to use for this
  ///                 execution.
  ///
  /// \returns - kSuccess on success; kRuntimeError if there is an error with
  ///            the runtime.
  ReturnCode evaluate(const Array<source_t> &sources,
                      const Array<target_t> &targets,
                      int refinement_limit, const method_t &method,
                      int n_digits, const std::vector<double> &kernelparams,
                      const distropolicy_t &distro = distropolicy_t { }) {
    // pack the arguments and call the action
    size_t n_params = kernelparams.size();
    size_t total_size = sizeof(EvaluateParams) + n_params * sizeof(double);
    EvaluateParams *args =
        reinterpret_cast<EvaluateParams *>(new char[total_size]);
    args->sources = sources;
    args->targets = targets;
    args->refinement_limit = refinement_limit;
    args->method = method;
    args->n_digits = n_digits;
    args->distro = distro;
    for (size_t i = 0; i < n_params; ++i) {
      args->kernelparams[i] = kernelparams[i];
    }

    if (HPX_SUCCESS != hpx_run(&evaluate_, nullptr, args, total_size)) {
      return kRuntimeError;
    }

    return kSuccess;
  }

 private:
  /// Action for evalutation
  static hpx_action_t evaluate_;

  /// Parameters to evaluations
  struct EvaluateParams {
    Array<source_t> sources;
    Array<target_t> targets;
    size_t refinement_limit;
    method_t method;
    int n_digits;
    distropolicy_t distro;
    double kernelparams[];
  };

  /// The evaluation action implementation
  // TODO: Recheck this - I think this should all work as is when the array is
  // all on one locality; the root locality.
  static int evaluate_handler(EvaluateParams *parms, size_t total_size) {
    // TODO: These need an update. We should decide if the tree creation
    // is called in a SPMD way, or in a diffusive way. This will make the
    // same source and target thing harder to work out correctly.
    sourceref_t sources = parms->sources.ref();
    targetref_t targets = parms->targets.ref();
    bool same_sandt = (sources.data() == targets.data()
                        && sources.n() == targets.n());;

    // TODO: The interface to tree will likely change.
    tree_t *tree = new tree_t{parms->method, parms->refinement_limit,
                              parms->n_digits};
    tree->partition(sources, targets, same_sandt);

    // Generate the table - TODO: this might be better as an action depending
    // on the domain geometry being computed. But that will have to wait on
    // the update to the interface of the tree.
    double domain_size = tree->domain()->size();
    size_t n_params = (total_size - sizeof(EvaluateParams)) / sizeof(double);
    std::vector<double> kernel_params(parms->kernelparams,
                                      &parms->kernelparams[n_params]);
    expansion_t::update_table(parms->n_digits, domain_size, kernel_params);

    // TODO: Here, each locality should do the DAG creation, so perhaps this
    // is SPMD, or should one thread launch the others?
    DAG dag = tree->create_DAG(same_sandt);
    parms->distro.compute_distribution(dag);

    tree->create_expansions_from_DAG();

    // NOTE: the previous has to finish for the following. So the previous
    // is a synchronous operation. The next three, however, are not. They all
    // get their work going when they come to it and then they return.

    tree->setup_edge_lists(dag);
    tree->start_DAG_evaluation();
    hpx_addr_t alldone = tree->setup_termination_detection(dag);

    hpx_lco_wait(alldone);
    hpx_lco_delete_sync(alldone);

    // clean up tree and table
    tree->destroy_DAG_LCOs(dag);
    delete tree;

    // return
    hpx_exit(0, nullptr);
  }
};


template <typename S, typename T,
          template <typename, typename> class E,
          template <typename, typename,
                    template <typename, typename> class,
                    typename> class M,
          typename D>
hpx_action_t Evaluator<S, T, E, M, D>::evaluate_ = HPX_ACTION_NULL;


} // namespace dashmm


#endif // __DASHMM_EVALUATOR_H__
