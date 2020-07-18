#include <chrono>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include "search.hpp"
#include "shogi.hpp"

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
  bool search_all = false;
  bool verbose = false;
  bool search_lance = false;
  u64 node_limit = Search::Unlimit;
  std::vector<std::string> piece_sets;

  for (int i = 1; i < argc; ++i) {
    const auto& arg = argv[i];
    if (std::strcmp(arg, "-a") == 0) {
      search_all = true;
    } else if (std::strcmp(arg, "-n") == 0) {
      ++i;
      if (i < argc) {
        node_limit = std::stoi(std::string(argv[i]));
      }
    } else if (std::strcmp(arg, "-v") == 0) {
      verbose = true;
    } else if (std::strcmp(arg, "-l") == 0) {
      search_lance = true;
    } else if (std::strcmp(arg, "--") == 0) {
      std::string line;
      while (std::getline(std::cin, line)) {
        piece_sets.push_back(line);
      }
    }else {
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
    PCVector pc_list = input_parse(piece_set);
    Search search(search_all, node_limit);
    search.set_verbose(verbose);
    search.set_search_lance(search_lance);

    int found_cnt = search.run(pc_list);
    std::printf("%s %d\n", piece_set.c_str(), found_cnt);
    if (found_cnt > 0) {
      std::printf("%s\n", search.ans_sfens()[0].c_str());
    }
  }

  return EXIT_SUCCESS;
}
