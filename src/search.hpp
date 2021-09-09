#ifndef KOMORI_SEARCH_HPP_
#define KOMORI_SEARCH_HPP_

#include <limits>
#include <map>
#include <string>
#include <vector>

#include "shogi.hpp"

namespace komori {
using PCVector = std::vector<PieceType>;

inline void PrintPcvector(const PCVector& pc_vector) {
  for (auto pc : pc_vector) {
    std::printf("%s", UsiString(pc));
  }
  std::putchar('\n');
}

struct SearchConfiguration {
  bool reverse_search{false};
  bool all_placement{false};
  bool lance_sensitive{false};
  bool verbose{false};

  u64 node_limit{std::numeric_limits<u64>::max()};
  u64 time_limit_ms{std::numeric_limits<u64>::max()};
};

class Search {
 public:
  static constexpr u64 Unlimit = std::numeric_limits<u64>::max();

  Search(const SearchConfiguration& config);
  Search(const Search&) = delete;
  Search(Search&&) = delete;
  Search& operator=(const Search&) = delete;
  Search& operator=(Search&&) = delete;
  ~Search(void) = default;

  int Run(const PCVector& pc_list);
  const std::vector<std::string>& AnsSfens(void) const { return ans_sfens_; }

 private:
  int RunUnreversible(const PCVector& pc_list);
  int RunReversible(const PCVector& pc_list);
  bool inferior_search_(const PCVector& pc_list);

  int SearchImpl(const PCVector& pc_list,
                 int pawn_b,
                 int pawn_v,
                 Bitboard no_control,
                 Bitboard pieces_bb,
                 int depth,
                 Square last_sq,
                 PiecePositions& pieces_log,
                 std::vector<std::string>& ans);

  bool search_both_dir_pawn_(const PCVector& pc_list,
                             int pawn,
                             int stone,
                             int lance,
                             Bitboard no_control,
                             Bitboard pieces_bb,
                             int depth,
                             Square last_sq,
                             PiecePositions& pieces_log,
                             std::vector<std::string>& ans);

  u64 node_count_{0};
  std::vector<std::string> ans_sfens_{};
  SearchConfiguration config_;
};

bool judge_placeable_dual(Bitboard no_effect_bb, int pawn, int stone, Bitboard pieces);

bool judge_placeable_dual_retboard(Bitboard no_effect_bb,
                                   int pawn,
                                   int stone,
                                   Bitboard pieces,
                                   Bitboard& pawn_bb,
                                   Bitboard& pawn_v_bb);
}  // namespace komori

#endif  // KOMORI_SEARCH_HPP_