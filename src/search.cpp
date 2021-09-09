#include <algorithm>
#include <cstdio>
#include <map>

#include "search.hpp"
#include "shogi.hpp"

using namespace komori;

namespace {
// Ordering pieces for placing
struct PCSortObject {
  bool operator()(const PieceType& l, const PieceType& r) const {
    // Queen -> ProRook -> ProBishop     (Strong Long Effect Piece)
    // -> Rook -> Bishop -> Lance        (Long Effect Piece)
    // -> King -> Gold, ProXXX -> Silver (Metal Piece)
    // -> Knight -> Pawn -> Stone        (Weak Piece)
    constexpr int table[PCNum] = {
        305,                 // BStone
        303, 110, 301, 213,  // BPawn, BLance, BKnight, BSilver
        108, 106, 203, 201,  // BBishop, BRook, BGold, BKing
        204, 205, 206, 207,  // BProGolds
        104, 102,            // BProBishop, BProRook
        0,                   // None
        305,                 // WStone
        304, 111, 302, 214,  // WPawn, WLance, WKnight, WSilver
        109, 107, 208, 202,  // WBishop, WRook, WGold, WKing
        209, 210, 211, 212,  // WProGolds
        105, 103,            // BProBishop, BProRook
        101,                 // Queen
        0,                   // None
    };
    return table[l] < table[r];
  }
};

template <bool Rev>
int count_pawn_like(const PCVector& pc_list) {
  return std::count_if(pc_list.begin(), pc_list.end(), [](const PieceType& pc) { return is_pawn_like<Rev>(pc); });
}

int count_pawn_like_either(const PCVector& pc_list) {
  return std::count_if(pc_list.begin(), pc_list.end(),
                       [](const PieceType& pc) { return is_pawn_like<false>(pc) || is_pawn_like<true>(pc); });
}

template <bool Rev>
Bitboard pawn_placeable(Bitboard no_effect_bb, Bitboard pieces_bb) {
  return no_effect_bb & ~pieces_bb.down<Rev>();
}
}  // namespace

namespace komori {
Search::Search(bool search_all, u64 node_limit)
    : node_count_(0), node_limit_(node_limit), search_all_(search_all), verbose_(false) {
  initAttackBB();
}

int Search::run(const PCVector& pc_list) {
  if (search_all_) {
    return run_all_(pc_list);
  } else {
    return run_single_(pc_list);
  }
}

int Search::run_single_(const PCVector& pc_list) {
  PCVector pc_list_sorted(pc_list);
  std::sort(pc_list_sorted.begin(), pc_list_sorted.end(), PCSortObject());

  int pawn = count_pawn_like<false>(pc_list_sorted);
  int pawn_v = count_pawn_like<true>(pc_list_sorted);
  PiecePositions pieces_log;
  return search_(pc_list_sorted, pawn, pawn_v, allOneBB(), allZeroBB(), 0, 0, pieces_log, ans_sfens_, false);
}

int Search::run_all_(const PCVector& pc_list) {
  PCVector symmetry_list;
  int asymmetry_len[PieceTypeNum] = {0};
  int asymmetry_cnt[PieceTypeNum] = {0};
  int pawn = 0;
  int lance = 0;
  int stone = 0;

  for (PieceType pc : pc_list) {
    PieceType pt = simplify_gold(pc2pt(pc));
    if (pt == Pawn) {
      // Lance is treated as Pawn in order to speed up
      pawn++;
    } else if (pt == Lance) {
      pawn++;
      lance++;
    } else if (pt == Stone) {
      stone++;
    } else if (is_symmetry(pt)) {
      symmetry_list.push_back(pt);
    } else {
      asymmetry_len[pt]++;
    }
  }

  // half of combination is skipped (because of horizonral symmetry)
  for (int i = PieceTypeNum - 1; i >= 0; --i) {
    if (asymmetry_len[i] > 0) {
      asymmetry_cnt[i] = (asymmetry_len[i] + 1) / 2;
      if (asymmetry_len[i] % 2 == 1) {
        break;
      }
    }
  }

  for (;;) {
    // merge (symmetry list) and (assymmetry list)
    PCVector pc_list(symmetry_list);
    for (int i = 0; i < PieceTypeNum; ++i) {
      for (int j = 0; j < asymmetry_cnt[i]; ++j) {
        pc_list.push_back(PieceType(i));
      }
      for (int j = asymmetry_cnt[i]; j < asymmetry_len[i]; ++j) {
        pc_list.push_back(PieceType(i | PTWhiteFlag));
      }
    }
    std::sort(pc_list.begin(), pc_list.end(), PCSortObject());
    if (verbose_) {
      for (PieceType pc : pc_list) {
        std::printf("%s", usi_string(pc));
      }
      std::putchar('\n');
    }

    int search_pawn = pawn + count_pawn_like_either(pc_list);
    int search_stone = stone + (pc_list.size() - count_pawn_like_either(pc_list));
    PiecePositions pieces_log;
    bool found;
    found = search_both_dir_pawn_(pc_list, search_pawn, search_stone, lance, allOneBB(), allZeroBB(), 0, 0, pieces_log,
                                  ans_sfens_);
    if (found) {
      if (verbose_) {
        std::printf("%s\n", ans_sfens_[0].c_str());
      }
      return 1;
    }

    // next combination
    for (int i = 0; i < PieceTypeNum; ++i) {
      if (asymmetry_len[i] > 0) {
        // check carry-up
        if (asymmetry_cnt[i] >= asymmetry_len[i]) {
          asymmetry_cnt[i] = 0;
        } else {
          asymmetry_cnt[i]++;
          break;
        }
      }

      // if (last combination) break;
      if (i == PieceTypeNum - 1) {
        return 0;
      }
    }
  }
}

int Search::search_(const PCVector& pc_list,
                    int pawn,
                    int pawn_v,
                    Bitboard no_effect_bb,
                    Bitboard pieces_bb,
                    int depth,
                    Square last_sq,
                    PiecePositions& pieces_log,
                    std::vector<std::string>& ans,
                    bool search) {
  int pc_len = static_cast<int>(pc_list.size());

  if (depth >= pc_len) {
    // found placement
    ans.push_back(pieces2sfen(pieces_log));
    std::cout << pieces2sfen(pieces_log) << std::endl;
    return 1;
  }

  // check limit of nodes
  ++node_count_;
  if (node_limit_ != Unlimit && node_count_ >= node_limit_) {
    return 0;
  }

  PieceType pc = pc_list[depth];
  Bitboard pawn_allowed = pawn_placeable<false>(no_effect_bb, pieces_bb);
  Bitboard pawn_allowed_v = pawn_placeable<true>(no_effect_bb, pieces_bb);
  Bitboard placeable_bb = no_effect_bb;
  // if (pc is the same of previous one)
  if (depth > 0 && pc_list[depth - 1] == pc) {
    // this piece should be placed after the previous one
    placeable_bb &= greaterMask(last_sq);
    if (pc == BlackPawn) {
      pawn_allowed &= greaterMask(last_sq);
    } else if (pc == WhitePawn) {
      pawn_allowed_v &= greaterMask(last_sq);
    }
  }

  int stone = pc_len - depth - pawn;
  int stone_v = pc_len - depth - pawn_v;
  // pawn-stone pruning
  if (!judge_placeable<false>(no_effect_bb, pawn, stone, pawn_allowed) ||
      !judge_placeable<true>(no_effect_bb, pawn_v, stone_v, pawn_allowed_v)) {
    return 0;
  }

  int found_cnt = 0;
  while (placeable_bb.isAny()) {
    Square sq = placeable_bb.firstOneFromSQ11();
    Bitboard attack = attackBB(pc, sq);
    if (!attack.andIsAny(pieces_bb)) {
      // placeable pc at sq
      pieces_log.emplace_back(pc, sq);

      // update #(pawn) and go to next depth
      int new_pawn = pawn - is_pawn_like<false>(pc);
      int new_pawn_v = pawn_v - is_pawn_like<true>(pc);
      found_cnt += search_(pc_list, new_pawn, new_pawn_v, no_effect_bb & (~attack), pieces_bb | squareMaskBB(sq),
                           depth + 1, sq, pieces_log, ans, search);

      pieces_log.pop_back();

      if (search && found_cnt) {
        break;
      }
    }
  }

  return found_cnt;
}

bool Search::search_both_dir_pawn_(const PCVector& pc_list,
                                   int pawn,
                                   int stone,
                                   int lance,
                                   Bitboard no_effect_bb,
                                   Bitboard pieces_bb,
                                   int depth,
                                   Square last_sq,
                                   PiecePositions& pieces_log,
                                   std::vector<std::string>& ans) {
  int pc_len = static_cast<int>(pc_list.size());

  if (depth >= pc_len) {
    Bitboard pawn_bb, pawn_v_bb;
    // judge if remain pawns and stones are placeable
    if (judge_placeable_dual_retboard(no_effect_bb, pawn, stone, pieces_bb, pawn_bb, pawn_v_bb)) {
      if (search_lance_ && ((pawn_bb & edgeBB(Black)) | (pawn_v_bb & edgeBB(White))).popCount() < lance) {
        return 0;
      }
      PiecePositions pieces_ans(pieces_log);
      // convert pawn_bb, pawn_v_bb to pieces_log entry
      while (pawn_bb.isAny()) {
        Square sq = pawn_bb.firstOneFromSQ11();
        pieces_ans.emplace_back(BlackPawn, sq);
      }
      while (pawn_v_bb.isAny()) {
        Square sq = pawn_v_bb.firstOneFromSQ11();
        pieces_ans.emplace_back(WhitePawn, sq);
      }
      ans.push_back(pieces2sfen(pieces_ans));
      return 1;
    } else {
      return 0;
    }
  }

  // check limit of nodes
  ++node_count_;
  if (node_limit_ != Unlimit && node_count_ >= node_limit_) {
    return 0;
  }

  // print #(nodes) periodically
  if (verbose_ && node_count_ % 100000000 == 0) {
    std::printf("%" PRIu64 " %d\n", node_count_, depth);
  }

  PieceType pc = pc_list[depth];
  Bitboard placeable_bb = no_effect_bb;
  if (depth > 0 && pc_list[depth - 1] == pc) {
    placeable_bb &= greaterMask(last_sq);
  }

  // pawn-stone purning
  if (!judge_placeable_dual(no_effect_bb, pawn, stone, pieces_bb)) {
    return 0;
  }

  while (placeable_bb.isAny()) {
    Square sq = placeable_bb.firstOneFromSQ11();
    Bitboard attack = attackBB(pc, sq);
    if (!attack.andIsAny(pieces_bb)) {
      // placeable pc at sq
      pieces_log.emplace_back(pc, sq);

      int new_pawn = pawn - (is_pawn_like<false>(pc) || is_pawn_like<true>(pc));
      int new_stone = stone - !(is_pawn_like<false>(pc) || is_pawn_like<true>(pc));
      bool found = search_both_dir_pawn_(pc_list, new_pawn, new_stone, lance, no_effect_bb & (~attack),
                                         pieces_bb | squareMaskBB(sq), depth + 1, sq, pieces_log, ans);

      pieces_log.pop_back();

      if (found) {
        return true;
      }
    }
  }

  return false;
}

template <bool Rev>
bool judge_placeable(Bitboard no_effect_bb, int pawn, int stone, Bitboard pawn_allowed) {
  // #(empty squares)
  int no_effect_num = no_effect_bb.popCount();

  // if (#(empty squares) < #(pieces to place)) not placeable;
  if (no_effect_num < pawn + stone) {
    return false;
  }

  // - alowwed to place pawn
  // - above is effected
  Bitboard next_placement = pawn_allowed & ~no_effect_bb.down<Rev>();
  int next_count = next_placement.popCount();
  // if (all pawns are placed in board)
  if (next_count >= pawn) {
    return true;
  }

  do {
    // update board
    pawn_allowed &= ~(next_placement | next_placement.up<Rev>() | next_placement.down<Rev>());
    no_effect_bb &= ~(next_placement | next_placement.up<Rev>());
    pawn -= next_count;

    // pack pawns from bottom of stage
    next_placement = pawn_allowed & ~no_effect_bb.up<Rev>();
    next_count = next_placement.popCount();
  } while (next_count > 0 && next_count < pawn);

  // (#(pawns to be placed) >= pawn) and (#(empty place) >= stone)
  return (next_count >= pawn && no_effect_bb.popCount() - 2 * pawn >= stone);
}

bool judge_placeable_dual(Bitboard no_effect_bb, int pawn, int stone, Bitboard pieces) {
  int no_effect_num = no_effect_bb.popCount();

  // if (#(empty squares) < #(pieces to place)) not placeable;
  if (no_effect_num < pawn + stone) {
    return false;
  }

  // place pawn whose above square is effected
  // - above is empty
  // - above is effected
  // - here is not effected
  Bitboard next_placement = ~pieces.down<false>() & ~no_effect_bb.down<false>() & no_effect_bb;
  Bitboard next_placement_v = ~pieces.down<true>() & ~no_effect_bb.down<true>() & ~next_placement & no_effect_bb;
  int next_count = next_placement.popCount() + next_placement_v.popCount();
  if (next_count >= pawn) {
    return true;
  }

  do {
    pieces |= next_placement | next_placement_v;
    // - pawn placed        -> piece exists
    // - pawn_v placed      -> piece exists
    // - (pawn placed).up   -> effect
    // - (pawn placed_v).up -> effect
    no_effect_bb &= ~(next_placement | next_placement_v | next_placement.up<false>() | next_placement_v.up<true>());
    pawn -= next_count;

    // condition:
    //     p
    //     (no piece)
    //     P
    //     (effect)
    // centinel is not needed because edge pawn is already placed if possible.
    next_placement =
        (~pieces.down<false>() & ~no_effect_bb.up<false>() & no_effect_bb & no_effect_bb.down<false>().down<false>());
    next_placement_v = next_placement.up<false>().up<false>();
    next_count = 2 * next_placement.popCount();
  } while (next_count > 0 && next_count < pawn);

  if (next_count >= pawn) {
    int space = no_effect_bb.popCount() - pawn / 2 * 3;
    if (pawn % 2 == 1) {
      space -= 2;
    }
    return space >= stone;
  }

  next_placement = (~pieces.down<false>() & ~no_effect_bb.up<false>() & no_effect_bb);
  next_count = next_placement.popCount();
  while (next_count > 0 && next_count < pawn) {
    pieces |= next_placement;
    no_effect_bb &= (next_placement & next_placement.up<false>());
    pawn -= next_count;

    next_placement = (~pieces.down<false>() & ~no_effect_bb.up<false>() & no_effect_bb);
    next_count = next_placement.popCount();
  }

  // (#(pawn placeable square) >= pawn) and (#(empty place) >= stone)
  return (next_count >= pawn && no_effect_bb.popCount() - 2 * pawn >= stone);
}

// @todo(komori): merge judge_placeable
bool judge_placeable_dual_retboard(Bitboard no_effect_bb,
                                   int pawn,
                                   int stone,
                                   Bitboard pieces,
                                   Bitboard& pawn_bb,
                                   Bitboard& pawn_v_bb) {
  int no_effect_num = no_effect_bb.popCount();
  pawn_bb = allZeroBB();
  pawn_v_bb = allZeroBB();

  // if (#(empty square) < #(pieces to place)) -> no placement
  if (no_effect_num < pawn + stone) {
    return false;
  }

  // place pawn whose above square is effected
  Bitboard next_placement = ~pieces.down<false>() & ~no_effect_bb.down<false>() & no_effect_bb;
  Bitboard next_placement_v = ~pieces.down<true>() & ~no_effect_bb.down<true>() & ~next_placement & no_effect_bb;
  int next_count = next_placement.popCount() + next_placement_v.popCount();
  if (next_count >= pawn) {
    pawn_bb |= next_placement;
    pawn_v_bb |= next_placement_v;
    return true;
  }

  do {
    pawn_bb |= next_placement;
    pawn_v_bb |= next_placement_v;
    pieces |= next_placement | next_placement_v;
    no_effect_bb &= ~(next_placement | next_placement_v | next_placement.up<false>() | next_placement_v.up<true>());
    pawn -= next_count;

    // centinel is not needed because edge pawn is already placed if possible.
    next_placement =
        (~pieces.down<false>() & ~no_effect_bb.up<false>() & no_effect_bb & no_effect_bb.down<false>().down<false>());
    next_placement_v = next_placement.up<false>().up<false>();
    next_count = 2 * next_placement.popCount();
  } while (next_count > 0 && next_count < pawn);

  if (next_count >= pawn) {
    int space = no_effect_bb.popCount() - pawn / 2 * 3;
    if (pawn % 2 == 1) {
      space -= 2;
    }
    if (space >= stone) {
      while (pawn > 0) {
        Square sq = next_placement.firstOneFromSQ11();
        pawn_bb |= squareMaskBB(sq);
        --pawn;
        if (pawn > 0) {
          pawn_v_bb |= squareMaskBB(sq).up<false>().up<false>();
          --pawn;
        }
      }

      return true;
    } else {
      return false;
    }
  }

  next_placement = (~pieces.down<false>() & ~no_effect_bb.up<false>() & no_effect_bb);
  next_count = next_placement.popCount();
  while (next_count > 0 && next_count < pawn) {
    pawn_bb |= next_placement;
    pawn_v_bb |= next_placement_v;
    pieces |= next_placement;
    no_effect_bb &= (next_placement & next_placement.up<false>());
    pawn -= next_count;

    next_placement = (~pieces.down<false>() & ~no_effect_bb.up<false>() & no_effect_bb);
    next_count = next_placement.popCount();
  }

  // (#(pawn placeable square) >= pawn) and (#(empty place) >= stone)
  if (next_count >= pawn && no_effect_bb.popCount() - 2 * pawn >= stone) {
    while (pawn > 0) {
      Square sq = next_placement.firstOneFromSQ11();
      pawn_bb |= squareMaskBB(sq);
      pawn--;
    }
    return true;
  } else {
    return false;
  }
}

// explicit instanciation
template bool judge_placeable<false>(Bitboard no_effect_bb, int pawn, int stone, Bitboard pawn_allowed);
template bool judge_placeable<true>(Bitboard no_effect_bb, int pawn, int stone, Bitboard pawn_allowed);

}  // namespace komori