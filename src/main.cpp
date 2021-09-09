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
  std::printf("usage: %s [-a] [-n node_limit] [-v] filename\n", argv[0]);
  std::printf("usage: %s [-a] [-n node_limit] [-v] --\n", argv[0]);
  std::printf("-a            : search all cases of piece reversing\n");
  std::printf("-n node_limit : node limits of searching\n");
  std::printf("-v            : verbose searching info\n");
  std::printf("-l            : don't replace lance into pawn(only effect with -a)\n");
  std::printf("--            : read from stdin\n");
  std::exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
  komori::SearchConfiguration config{};
  std::vector<std::string> piece_sets;

  for (int i = 1; i < argc; ++i) {
    const auto& arg = argv[i];
    if (std::strcmp(arg, "-a") == 0) {
      config.all_placement = true;
    } else if (std::strcmp(arg, "-n") == 0) {
      ++i;
      if (i < argc) {
        config.node_limit = std::stoi(std::string{argv[i]});
      }
    } else if (std::strcmp(arg, "-v") == 0) {
      config.verbose = true;
    } else if (std::strcmp(arg, "-l") == 0) {
      config.lance_sensitive = true;
    } else if (std::strcmp(arg, "--") == 0) {
      std::string line;
      while (std::getline(std::cin, line)) {
        piece_sets.push_back(line);
      }
    } else {
      std::ifstream ifs(arg);
      if (!ifs) {
        std::printf("file open error: %s\n\n", arg);
        help_and_exit(argc, argv);
      }
      std::string line;
      while (std::getline(ifs, line)) {
        piece_sets.push_back(line);
      }
    }
  }

  if (piece_sets.size() == 0) {
    help_and_exit(argc, argv);
  }

  for (std::string piece_set : piece_sets) {
    PCVector pc_list = InputParse(piece_set);
    Search search(config);

    int found_cnt = search.Run(pc_list);
    std::printf("%s %d\n", piece_set.c_str(), found_cnt);
    if (found_cnt > 0) {
      for (const auto& sfen : search.AnsSfens()) {
        std::cout << sfen << std::endl;
      }
    }
  }

  return EXIT_SUCCESS;
}
