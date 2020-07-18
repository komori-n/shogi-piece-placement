#pragma once

#include <vector>
#include <string>
#include <map>
#include <limits>

#include "shogi.hpp"

using PCVector = std::vector<PieceType>;

inline void print_pcvector(const PCVector& pc_vector) {
  for (auto pc : pc_vector) {
    std::printf("%s", usi_string(pc));
  }
  std::putchar('\n');
}

class Search {
 public:
  static constexpr u64 Unlimit = std::numeric_limits<u64>::max();

  Search(bool search_all=false, u64 node_limit=Unlimit);
  int run(const PCVector& pc_list);
  void set_verbose(bool verbose) { verbose_ = verbose; }
  void set_search_lance(bool search_lance) { search_lance_ = search_lance; }

  const std::vector<std::string>& ans_sfens(void) const { return ans_sfens_; }

 private:
  int run_single_(const PCVector& pc_list);
  int run_all_(const PCVector& pc_list);
  bool inferior_search_(const PCVector& pc_list);

  int search_(
    const PCVector& pc_list,
    int pawn, int pawn_v,
    Bitboard no_control, Bitboard pieces_bb,
    int depth, Square last_sq,
    PiecePositions& pieces_log,
    std::vector<std::string>& ans,
    bool search
  );

  bool search_both_dir_pawn_(
    const PCVector& pc_list,
    int pawn, int stone, int lance,
    Bitboard no_control, Bitboard pieces_bb,
    int depth, Square last_sq,
    PiecePositions& pieces_log,
    std::vector<std::string>& ans
  );

  u64 node_count_;
  u64 node_limit_;
  bool search_all_;
  bool verbose_;
  bool search_lance_;

  std::vector<std::string> ans_sfens_;
};

template <bool Rev>
bool judge_placeable(Bitboard no_effect_bb,
    int pawn, int stone,
    Bitboard pawn_allowed);

bool judge_placeable_dual(Bitboard no_effect_bb,
    int pawn, int stone,
    Bitboard pieces);

bool judge_placeable_dual_retboard(Bitboard no_effect_bb,
    int pawn, int stone,
    Bitboard pieces, Bitboard& pawn_bb, Bitboard& pawn_v_bb);