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
  std::printf("--            : read from stdin\n");
  std::exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
  bool search_all = false;
  bool verbose = false;
  u64 node_limit = Search::Unlimit;

  std::string in_str;
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
    } else if (std::strcmp(arg, "--") == 0) {
      in_str = std::string(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>());
    }else {
      if (!in_str.empty()) {
        help_and_exit(argc, argv);
      }

      std::ifstream ifs(arg);
      if (!ifs) {
        std::printf("file open error: %s\n\n", arg);
        help_and_exit(argc, argv);
      }
      in_str = std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
    }
  }

  if (in_str.empty()) {
    help_and_exit(argc, argv);
  }

  PCVector pc_list = input_parse(in_str);
  Search search(search_all, node_limit);
  search.set_verbose(verbose);

  int found_cnt = search.run(pc_list);

  std::printf("%d\n", found_cnt);
  if (found_cnt > 0) {
    std::printf("%s\n", search.ans_sfens()[0].c_str());
  }

  return EXIT_SUCCESS;
}
