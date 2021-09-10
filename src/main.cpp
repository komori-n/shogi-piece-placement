#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

#include "search.hpp"
#include "shogi.hpp"

using namespace komori;

void help_and_exit(int argc, char* argv[]) {
  std::printf("usage: %s [-a] [-n node_limit] [-v] sfen\n", argv[0]);
  std::printf("usage: %s [-a] [-n node_limit] [-v] --\n", argv[0]);
  std::printf("-a            : find all solutions (may take very long time");
  std::printf("-b            : consider piece reverse\n");
  std::printf("-n node_limit : node limits of searching\n");
  std::printf("--            : read from stdin\n");
  std::exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
  komori::SearchConfiguration config{};
  std::string piece_set;

  for (int i = 1; i < argc; ++i) {
    const auto& arg = argv[i];
    if (std::strcmp(arg, "-a") == 0) {
      config.all_placement = true;
    } else if (std::strcmp(arg, "-b") == 0) {
      config.reverse_search = true;
    } else if (std::strcmp(arg, "-n") == 0) {
      ++i;
      if (i < argc) {
        config.node_limit = std::stoi(std::string{argv[i]});
      }
    } else if (std::strcmp(arg, "--") == 0) {
      std::cin >> piece_set;
    } else {
      piece_set = arg;
    }
  }

  if (piece_set.empty()) {
    help_and_exit(argc, argv);
  }

  PCVector pc_list = InputParse(piece_set);
  Search search(config);

  int found_cnt = search.Run(pc_list);
  if (found_cnt > 0) {
    for (const auto& sfen : search.AnsSfens()) {
      std::cout << sfen << std::endl;
    }

    if (config.all_placement) {
      std::cout << "found " << found_cnt << " solutions" << std::endl;
    }
  } else {
    std::cout << "not found" << std::endl;
  }

  return EXIT_SUCCESS;
}
