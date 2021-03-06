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


#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <getopt.h>
#include <sys/time.h>

#include <algorithm>
#include <complex>
#include <map>
#include <memory>
#include <string>
#include "dashmm/dashmm.h"

// The type used for source data. This meets all the requiremenets of the
// various expansions used in this demo.
struct SourceData {
  dashmm::Point position;
  double charge;
};

// The type used for target data. This meets all the requirements of the
// various expansions used in this demo. Additionally, an index is stored
// to make it easy to compare with the exact result.
struct TargetData {
  dashmm::Point position;
  std::complex<double> phi;
  int index;
};

// Here we create the evaluator objects that we shall need in this demo.
// These must be instantiated before the call to dashmm::init so that they
// might register the relevant actions with the runtime system.
//
// It is more typical to have only one or possibly two evaluators. The
// proliferation here is because this demo program allows for many use-cases.
dashmm::Evaluator<SourceData, TargetData,
                  dashmm::LaplaceCOM, dashmm::BH> laplace_bh{};
dashmm::Evaluator<SourceData, TargetData,
                  dashmm::LaplaceCOM, dashmm::Direct> laplace_direct{};
dashmm::Evaluator<SourceData, TargetData,
                  dashmm::Laplace, dashmm::FMM> laplace_fmm{};
dashmm::Evaluator<SourceData, TargetData,
                  dashmm::Laplace, dashmm::FMM97> laplace_fmm97{};
dashmm::Evaluator<SourceData, TargetData,
                  dashmm::Yukawa, dashmm::Direct> yukawa_direct{};
dashmm::Evaluator<SourceData, TargetData,
                  dashmm::Yukawa, dashmm::FMM97> yukawa_fmm97{};
dashmm::Evaluator<SourceData, TargetData,
                  dashmm::Helmholtz, dashmm::Direct> helmholtz_direct{};
dashmm::Evaluator<SourceData, TargetData,
                  dashmm::Helmholtz, dashmm::FMM97> helmholtz_fmm97{};

// This type collects the input arguments to the program.
struct InputArguments {
  int source_count;
  std::string source_type;
  int target_count;
  std::string target_type;
  int refinement_limit;
  std::string method;
  std::string kernel;
  bool verify;
  int accuracy;
};

// Print usage information.
void print_usage(char *progname) {
  fprintf(stdout, "Usage: %s [OPTIONS]\n\n"
          "Options available: [possible/values] (default value)\n"
          "--method=[fmm/fmm97/bh]     method to use (fmm)\n"
          "--nsources=num              "
          "number of source points to generate (10000)\n"
          "--sourcedata=[cube/sphere/plummer]\n"
          "                            source distribution type (cube)\n"
          "--ntargets=num              "
          "number of target points to generate (10000)\n"
          "--targetdata=[cube/sphere/plummer]\n"
          "                            target distribution type (cube)\n"
          "--threshold=num             "
          "source and target tree partition refinement limit (40)\n"
          "--accuracy=num              "
          "number of digits of accuracy for fmm (3)\n"
          "--verify=[yes/no]           "
          "perform an accuracy test comparing to direct summation (yes)\n"
          "--kernel=[laplace/yukawa/helmholtz]   "
          "particle interaction type (laplace)\n"
          , progname);
}

// Parse the command line arguments, overiding any defaults at the request of
// the user.
int read_arguments(int argc, char **argv, InputArguments &retval) {
  //Set defaults
  retval.source_count = 10000;
  retval.source_type = std::string{"cube"};
  retval.target_count = 10000;
  retval.target_type = std::string{"cube"};
  retval.refinement_limit = 40;
  retval.method = std::string{"fmm97"};
  retval.kernel = std::string{"laplace"};
  retval.verify = true;
  retval.accuracy = 3;

  int opt = 0;
  static struct option long_options[] = {
    {"method", required_argument, 0, 'm'},
    {"nsources", required_argument, 0, 's'},
    {"sourcedata", required_argument, 0, 'w'},
    {"ntargets", required_argument, 0, 't'},
    {"targetdata", required_argument, 0, 'g'},
    {"threshold", required_argument, 0, 'l'},
    {"verify", required_argument, 0, 'v'},
    {"accuracy", required_argument, 0, 'a'},
    {"kernel", required_argument, 0, 'k'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
  };

  int long_index = 0;
  while ((opt = getopt_long(argc, argv, "m:s:w:t:g:l:v:a:k:h",
                            long_options, &long_index)) != -1) {
    std::string verifyarg{};
    switch (opt) {
    case 'm':
      retval.method = optarg;
      break;
    case 's':
      retval.source_count = atoi(optarg);
      break;
    case 'w':
      retval.source_type = optarg;
      break;
    case 't':
      retval.target_count = atoi(optarg);
      break;
    case 'g':
      retval.target_type = optarg;
      break;
    case 'l':
      retval.refinement_limit = atoi(optarg);
      break;
    case 'v':
      verifyarg = optarg;
      retval.verify = (verifyarg == std::string{"yes"});
      break;
    case 'a':
      retval.accuracy = atoi(optarg);
      break;
    case 'k':
      retval.kernel = optarg;
      break;
    case 'h':
      print_usage(argv[0]);
      return -1;
    case '?':
      return -1;
    }
  }

  //test the inputs
  if (retval.source_count < 1) {
    fprintf(stderr, "Usage ERROR: nsources must be positive.\n");
    return -1;
  }

  if (retval.target_count < 1) {
    fprintf(stderr, "Usage ERROR: ntargets must be positive\n");
    return -1;
  }

  if (retval.method != "bh" && retval.method != "fmm"
      && retval.method != "fmm97") {
    fprintf(stderr, "Usage ERROR: unknown method '%s'\n",
            retval.method.c_str());
    return -1;
  }

  if (retval.source_type != "cube" && retval.source_type != "sphere"
        && retval.source_type != "plummer") {
    fprintf(stderr, "Usage ERROR: unknown source type '%s'\n",
            retval.source_type.c_str());
    return -1;
  }

  if (retval.target_type != "cube" && retval.target_type != "sphere"
        && retval.target_type != "plummer") {
    fprintf(stderr, "Usage ERROR: unknown target type '%s'\n",
            retval.target_type.c_str());
    return -1;
  }

  if (retval.kernel == "laplace" && retval.method == "fmm97") {
    if (retval.accuracy != 3 && retval.accuracy != 6) {
      fprintf(stderr, "Usage ERROR: only 3-/6-digit accuracy supported"
              " for laplace kernel using fmm97\n");
      return -1;
    }
  }

  if (retval.kernel == "yukawa") {
    if (retval.method != "fmm97") {
      fprintf(stderr, "Usage ERROR: yukawa kernel must use fmm97\n");
      return -1;
    } else if (retval.accuracy != 3 && retval.accuracy != 6) {
      fprintf(stderr, "Usage ERROR: only 3-/6-digit accuracy supported"
              " for yukawa kernel using fmm97\n");
      return -1;
    }
  }

  if (retval.kernel == "helmholtz") {
    if (retval.method != "fmm97") {
      fprintf(stderr, "Usage ERROR: helmholtz kernel must use fmm97\n");
      return -1;
    } else if (retval.accuracy != 3 && retval.accuracy != 6) {
      fprintf(stderr, "Usage ERROR: only 3-/6-digit accuracy supported"
              " for helmholtz kernel using fmm97\n");
      return -1;
    }
  }

  //print out summary
  if (dashmm::get_my_rank() == 0) {
    fprintf(stdout, "Testing DASHMM:\n");
    fprintf(stdout, "%d sources in a %s distribution\n",
            retval.source_count, retval.source_type.c_str());
    fprintf(stdout, "%d targets in a %s distribution\n",
            retval.target_count, retval.target_type.c_str());
    fprintf(stdout, "method: %s \nthreshold: %d\nkernel: %s\n\n",
            retval.method.c_str(), retval.refinement_limit,
            retval.kernel.c_str());
  } else {
    // Only have rank 0 create data
    retval.source_count = 0;
    retval.target_count = 0;
  }

  return 0;
}

// Used to time the execution
inline double getticks(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (double) (tv.tv_sec * 1e6 + tv.tv_usec);
}

inline double elapsed(double t1, double t0) {
  return (double) (t1 - t0);
}

// Pick the positions in a cube with a uniform distribution
dashmm::Point pick_cube_position() {
  double pos[3];
  pos[0] = (double)rand() / RAND_MAX - 0.5;
  pos[1] = (double)rand() / RAND_MAX - 0.5;
  pos[2] = (double)rand() / RAND_MAX - 0.5;
  return dashmm::Point{pos[0], pos[1], pos[2]};
}

// Pick a point from the surface of the sphere, with a uniform distribution
dashmm::Point pick_sphere_position() {
  double r = 1.0;
  double ctheta = 2.0 * (double)rand() / RAND_MAX - 1.0;
  double stheta = sqrt(1.0 - ctheta * ctheta);
  double phi = 2.0 * 3.1415926535 * (double)rand() / RAND_MAX;
  double pos[3];
  pos[0] = r * stheta * cos(phi);
  pos[1] = r * stheta * sin(phi);
  pos[2] = r * ctheta;
  return dashmm::Point{pos[0], pos[1], pos[2]};
}

// Pick a random charge value
double pick_charge(bool use_negative) {
  double retval = (double)rand() / RAND_MAX + 1.0;
  if (use_negative && (rand() % 2)) {
    retval *= -1.0;
  }
  return retval;
}

// Pick a position in the plummer distribution
dashmm::Point pick_plummer_position() {
  //NOTE: This is using a = 1
  double unif = (double)rand() / RAND_MAX;
  double r = 1.0 / sqrt(pow(unif, -2.0 / 3.0) - 1);
  double ctheta = 2.0 * (double)rand() / RAND_MAX - 1.0;
  double stheta = sqrt(1.0 - ctheta * ctheta);
  double phi = 2.0 * 3.1415926535 * (double)rand() / RAND_MAX;
  double pos[3];
  pos[0] = r * stheta * cos(phi);
  pos[1] = r * stheta * sin(phi);
  pos[2] = r * ctheta;
  return dashmm::Point{pos[0], pos[1], pos[2]};
}

// Set the charges in the plummer case to be constant, and consistent with the
// distribution.
double pick_plummer_charge(int count) {
  //We take the total mass to be 100
  return 100.0 / count;
}

// Set the source data
void set_sources(SourceData *sources, int source_count,
                 std::string source_type, std::string method) {
  bool use_negative = (method != std::string{"bh"});
  if (source_type == std::string{"cube"}) {
    for (int i = 0; i < source_count; ++i) {
      sources[i].position = pick_cube_position();
      sources[i].charge = pick_charge(use_negative);
    }
  } else if (source_type == std::string{"sphere"}) {
    //Sphere
    for (int i = 0; i < source_count; ++i) {
      sources[i].position = pick_sphere_position();
      sources[i].charge = pick_charge(use_negative);
    }
  } else {
    //Plummer
    for (int i = 0; i < source_count; ++i) {
      sources[i].position = pick_plummer_position();
      sources[i].charge = pick_plummer_charge(source_count);
    }
  }
}

// Prepare the source data. This means both allocate the dashmm::Array,
// as well as creating the source data, and putting the source data into the
// global address space.
dashmm::Array<SourceData> prepare_sources(InputArguments &args) {
  SourceData *sources{nullptr};
  if (args.source_count) {
    sources = new SourceData[args.source_count];
    set_sources(sources, args.source_count, args.source_type, args.method);
  }

  dashmm::Array<SourceData> retval{};
  int err = retval.allocate(args.source_count);
  assert(err == dashmm::kSuccess);
  err = retval.put(0, args.source_count, sources);
  assert(err == dashmm::kSuccess);

  if (args.source_count) {
    delete [] sources;
  }

  return retval;
}

// Set the target data
void set_targets(TargetData *targets, int target_count,
                 std::string target_type) {
  if (target_type == std::string{"cube"}) {
    //Cube
    for (int i = 0; i < target_count; ++i) {
      targets[i].position = pick_cube_position();
      targets[i].phi = 0.0;
      targets[i].index = i;
    }
  } else if (target_type == std::string{"sphere"}) {
    //Sphere
    for (int i = 0; i < target_count; ++i) {
      targets[i].position = pick_sphere_position();
      targets[i].phi = 0.0;
      targets[i].index = i;
    }
  } else {
    //Plummer
    for (int i = 0; i < target_count; ++i) {
      targets[i].position = pick_plummer_position();
      targets[i].phi = 0.0;
      targets[i].index = i;
    }
  }
}

// Prepare the target data. This means both allocate the dashmm::Array,
// as well as creating the target data, and putting the target data into the
// global address space.
dashmm::Array<TargetData> prepare_targets(InputArguments &args) {
  TargetData *targets{nullptr};
  if (args.target_count) {
    targets = new TargetData[args.target_count];
    set_targets(targets, args.target_count, args.target_type);
  }

  dashmm::Array<TargetData> retval{};
  int err = retval.allocate(args.target_count);
  assert(err == dashmm::kSuccess);
  err = retval.put(0, args.target_count, targets);
  assert(err == dashmm::kSuccess);

  if (args.target_count) {
    delete [] targets;
  }

  return retval;
}

// Compute an error characteristic for the values computed with a multipole
// method, and the values computed with direct summation.
void compare_results(TargetData *targets, int target_count,
                     TargetData *exacts, int exact_count) {
  if (dashmm::get_my_rank()) return;

  //create a map from index into offset for targets
  std::map<int, int> offsets{};
  for (int i = 0; i < target_count; ++i) {
    offsets[targets[i].index] = i;
  }

  //now we loop over the exact results and compare
  double numerator = 0.0;
  double denominator = 0.0;
  double maxrel = 0.0;
  for (int i = 0; i < exact_count; ++i) {
    auto j = offsets.find(exacts[i].index);
    assert(j != offsets.end());
    int idx = j->second;
    double relerr = std::norm(targets[idx].phi - exacts[i].phi);
    numerator += relerr;
    double exnorm = std::norm(exacts[i].phi);
    denominator += exnorm;
    if (sqrt(relerr / exnorm) > maxrel) {
      maxrel = sqrt(relerr / exnorm);
    }
  }
  fprintf(stdout, "Error for %d test points: %4.3e (max %4.3e)\n",
                  exact_count, sqrt(numerator / denominator), maxrel);
}

// The main driver routine that performes the test of evaluate()
void perform_evaluation_test(InputArguments args) {
  srand(123456);

  dashmm::Array<SourceData> source_handle = prepare_sources(args);
  dashmm::Array<TargetData> target_handle = prepare_targets(args);

  //Perform the evaluation
  double t0{};
  double tf{};
  int err{0};

  if (args.kernel == std::string{"laplace"}) {
    if (args.method == std::string{"bh"}) {
      dashmm::BH<SourceData, TargetData, dashmm::LaplaceCOM> method{0.6};

      t0 = getticks();
      err = laplace_bh.evaluate(source_handle, target_handle,
                                args.refinement_limit, method,
                                args.accuracy, std::vector<double>{});
      assert(err == dashmm::kSuccess);
      tf = getticks();
    } else if (args.method == std::string{"fmm"}) {
      dashmm::FMM<SourceData, TargetData, dashmm::Laplace> method{};

      t0 = getticks();
      err = laplace_fmm.evaluate(source_handle, target_handle,
                                 args.refinement_limit, method,
                                 args.accuracy, std::vector<double>{});
      assert(err == dashmm::kSuccess);
      tf = getticks();
    } else if (args.method == std::string{"fmm97"}) {
      dashmm::FMM97<SourceData, TargetData, dashmm::Laplace> method{};

      t0 = getticks();
      err = laplace_fmm97.evaluate(source_handle, target_handle,
                                   args.refinement_limit, method,
                                   args.accuracy, std::vector<double>{});
      assert(err == dashmm::kSuccess);
      tf = getticks();
    }
  } else if (args.kernel == std::string{"yukawa"}) {
    if (args.method == std::string{"fmm97"}) {
      dashmm::FMM97<SourceData, TargetData, dashmm::Yukawa> method{};
      std::vector<double> kernelparms(1, 0.1);

      t0 = getticks();
      err = yukawa_fmm97.evaluate(source_handle, target_handle,
                                  args.refinement_limit, method,
                                  args.accuracy, kernelparms);
      assert(err == dashmm::kSuccess);
      tf = getticks();
    }
  } else if (args.kernel == std::string{"helmholtz"}) {
    if (args.method == std::string{"fmm97"}) {
      dashmm::FMM97<SourceData, TargetData, dashmm::Helmholtz> method{};
      std::vector<double> kernelparms(1, 0.1);

      t0 = getticks();
      err = helmholtz_fmm97.evaluate(source_handle, target_handle,
                                     args.refinement_limit, method,
                                     args.accuracy, kernelparms);
      assert(err == dashmm::kSuccess);
      tf = getticks();
    }
  }

  fprintf(stdout, "Evaluation took %lg [us]\n", elapsed(tf, t0));

  if (args.verify) {
    // Save a few targets for the direct comparison
    int test_count{0};
    if (hpx_get_my_rank() == 0) {
      test_count = 400;
    }
    if (test_count > args.target_count) {
      test_count = args.target_count;
    }

    //Get the results from the global address space
    args.target_count = target_handle.length();
    TargetData *targets = target_handle.collect();
    if (targets) {
      std::sort(targets, &targets[args.target_count],
                [] (const TargetData &a, const TargetData &b) -> bool {
                  return (a.index < b.index);
                });
    }

    // Copy the test particles into test_targets
    TargetData *test_targets{nullptr};
    if (test_count) {
      test_targets = new TargetData[test_count];

      for (int i = 0; i < test_count; ++i) {
        int idx = i * (args.target_count / test_count);
        assert(idx < args.target_count);
        test_targets[i] = targets[idx];
        test_targets[i].phi = std::complex<double>{0.0, 0.0};
      }
    }

    // Create array for test targets
    dashmm::Array<TargetData> test_handle{};
    err = test_handle.allocate(test_count);
    assert(err == dashmm::kSuccess);
    err = test_handle.put(0, test_count, test_targets);
    assert(err == dashmm::kSuccess);
    delete [] test_targets;

    //do direct evaluation
    if (args.kernel == "laplace") {
      dashmm::Direct<SourceData, TargetData, dashmm::LaplaceCOM> direct{};
      err = laplace_direct.evaluate(source_handle, test_handle,
                                    args.refinement_limit, direct,
                                    args.accuracy, std::vector<double>{});
      assert(err == dashmm::kSuccess);
    } else if (args.kernel == "yukawa") {
      dashmm::Direct<SourceData, TargetData, dashmm::Yukawa> direct{};
      std::vector<double> kernelparms(1, 0.1);
      err = yukawa_direct.evaluate(source_handle, test_handle,
                                   args.refinement_limit, direct,
                                   args.accuracy, kernelparms);
      assert(err == dashmm::kSuccess);
    } else if (args.kernel == "helmholtz") {
      dashmm::Direct<SourceData, TargetData, dashmm::Helmholtz> direct{};
      std::vector<double> kernelparms(1, 0.1);
      err = helmholtz_direct.evaluate(source_handle, test_handle,
                                      args.refinement_limit, direct,
                                      args.accuracy, kernelparms);
      assert(err == dashmm::kSuccess);
    }

    // Retrieve the test results
    test_targets = test_handle.collect();

    //Test error
    compare_results(targets, args.target_count, test_targets, test_count);

    err = test_handle.destroy();
    assert(err == dashmm::kSuccess);
    delete [] test_targets;
    delete [] targets;
  }

  //free up resources
  err = source_handle.destroy();
  assert(err == dashmm::kSuccess);
  err = target_handle.destroy();
  assert(err == dashmm::kSuccess);
}

// Program entrypoint
int main(int argc, char **argv) {
  auto err = dashmm::init(&argc, &argv);
  assert(err == dashmm::kSuccess);

  InputArguments inputargs;
  int usage_error = read_arguments(argc, argv, inputargs);

  if (!usage_error) {
    perform_evaluation_test(inputargs);
  }

  err = dashmm::finalize();
  assert(err == dashmm::kSuccess);

  return 0;
}
