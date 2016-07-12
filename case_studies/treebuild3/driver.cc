#include <iostream>
#include <getopt.h>
#include "tree.h"

void usage(std::string exec) {
  std::cout << "Usage: " << exec 
            << "--datatype=[c/s] " 
            << "--nsrc-per-rank=num " 
            << "--ntar-per-rank=num "
            << "--threshold=num "
            << std::endl; 
}

int main(int argc, char *argv[]) {
  // default values 
  char datatype = 'c';
  int nsrc_per_rank = 1; 
  int ntar_per_rank = 1; 
  int threshold = 1; 
 
  if (hpx_init(&argc, &argv)) {
    std::cout << "HPX: failed to initialize" << std::endl;
  }

  // Parse command line 
  int opt = 0; 
  static struct option long_options[] = {
    {"datatype", required_argument, 0, 'd'}, 
    {"nsrc-per-rank", required_argument, 0, 's'}, 
    {"ntar-per-rank", required_argument, 0, 't'}, 
    {"threshold", required_argument, 0, 'l'}, 
    {"help", no_argument, 0, 'h'}, 
    {0, 0, 0, 0}
  }; 

  int long_index = 0; 
  while ((opt = getopt_long(argc, argv, "s:t:l:d:h", 
                            long_options, &long_index)) != -1) {
    switch (opt) {
    case 's':
      nsrc_per_rank = atoi(optarg); 
      break;
    case 't':
      ntar_per_rank = atoi(optarg); 
      break;
    case 'l':
      threshold = atoi(optarg); 
      break;
    case 'd':
      datatype = *optarg;
      break;
    case 'h':
      usage(std::string{argv[0]});
      return -1;
    }
  }

  hpx_addr_t sources{HPX_NULL}; 
  hpx_addr_t targets{HPX_NULL}; 
  hpx_addr_t *sources_addr = &sources; 
  hpx_addr_t *targets_addr = &targets; 

  if (hpx_run(&allocate_points_action, &nsrc_per_rank, &ntar_per_rank, 
              &datatype, &sources_addr, &targets_addr)) {
    std::cout << "Failed to allocate points" << std::endl; 
  }
  
  if (hpx_run(&partition_points_action, &nsrc_per_rank, &sources, 
              &ntar_per_rank, &targets, &threshold)) {
    std::cout << "Failed to partition points" << std::endl; 
  }

  if (hpx_run(&delete_points_action, &sources, &targets)) {
    std::cout << "Failed to delete points" << std::endl;
  }

  hpx_finalize(); 
  return 0;
}
