#include <algorithm>
#include <cstdio>
#include <map>
#include <mutex>

#include "search.hpp"
#include "shogi.hpp"

using namespace komori;

namespace {
/**
 * @brief A function object to sort a PCVector by decending order of strength
 *
 * The placement of stronger pieces (whch have more effects) first will make that of rests easier.
 * This structs enables to sort PCVector by strength ordering heuristically.
 */
struct PCSortObject {
  bool operator()(const PieceType& l, const PieceType& r) const {
    // Queen -> ProRook -> ProBishop     (Strong Long Effect Piece)
    // -> Rook -> Bishop -> Lance        (Long Effect Piece)
    // -> King -> Gold, ProXXX -> Silver (Metal Piece)
    // -> Knight -> Pawn -> Stone        (Weak Piece)
    constexpr int kValueTable[PCNum] = {
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
    return kValueTable[l] < kValueTable[r];
  }
};

/// Count the number of pawn-like pieces
template <Color C>
int CountPawnLike(const PCVector& pc_list) {
  return std::count_if(pc_list.begin(), pc_list.end(), [](const PieceType& pc) { return IsPawnLike<C>(pc); });
}

/// Count the number of pawn-like pieces (both black pawn and white pawn)
int CountPawnLikeEither(const PCVector& pc_list) {
  return std::count_if(pc_list.begin(), pc_list.end(),
                       [](const PieceType& pc) { return IsPawnLike<Black>(pc) || IsPawnLike<White>(pc); });
}

template <Color C>
Bitboard PawnPlaceable(Bitboard no_effect_bb, Bitboard pieces_bb) {
  return no_effect_bb & ~pieces_bb.down<C>();
}

/// Judge if `pawn`s and `stone`s are placeable in `no_effect_bb`
template <Color C>
bool JudgePlaceable(Bitboard no_effect_bb, int pawn, int stone, Bitboard pawn_allowed) {
  int empty_num = no_effect_bb.popCount();

  if (empty_num < pawn + stone) {
    return false;
  }

  // A bitboard which is allowed to place pawn and is not effected by other pieces
  Bitboard next_placement = pawn_allowed & ~no_effect_bb.down<C>();
  int next_count = next_placement.popCount();
  if (next_count >= pawn) {
    return true;
  }

  do {
    // Place pawns to `next_placement`
    pawn_allowed &= ~(next_placement | next_placement.up<C>() | next_placement.down<C>());
    no_effect_bb &= ~(next_placement | next_placement.up<C>());
    pawn -= next_count;

    // Recalculate next placement bitboard
    next_placement = pawn_allowed & ~no_effect_bb.up<C>();
    next_count = next_placement.popCount();
  } while (next_count > 0 && next_count < pawn);

  // (#(pawns to be placed) >= pawn) and (#(empty place) >= stone)
  return (next_count >= pawn && no_effect_bb.popCount() - 2 * pawn >= stone);
}

/// Judge if `pawn's and `stone`s are placeable in `no_effect_bb` (both direction is ok)
bool JudgeNonDirectionalPlacement(Bitboard no_effect_bb, int pawn, int stone, Bitboard pieces) {
  int empty_num = no_effect_bb.popCount();

  if (empty_num < pawn + stone) {
    return false;
  }

  // Case 1: The abose square is effected.
  //   One pawns can be places per an empty square

  // Place pawns as many as possible on the squares applying the following all.
  // - above is empty
  // - above is effected
  // - here is not effected
  Bitboard next_placement_b = ~pieces.down<Black>() & ~no_effect_bb.down<Black>() & no_effect_bb;
  Bitboard next_placement_w = ~pieces.down<White>() & ~no_effect_bb.down<White>() & ~next_placement_b & no_effect_bb;
  int next_count = next_placement_b.popCount() + next_placement_w.popCount();
  if (next_count >= pawn) {
    return true;
  }

  // Case 2: 3 consecutive squares are empty.
  //   Two pawns can be placed per three empty squares
  do {
    // Place pawns to `next_placement_b` and `next_placement_w`
    pieces |= next_placement_b | next_placement_w;
    // - pawn placed        -> piece exists
    // - pawn_w placed      -> piece exists
    // - (pawn placed).up   -> effect
    // - (pawn placed_v).up -> effect
    no_effect_bb &=
        ~(next_placement_b | next_placement_w | next_placement_b.up<Black>() | next_placement_w.up<White>());
    pawn -= next_count;

    // condition:
    //     p
    //     (no piece)
    //     P
    //     (effect)
    // centinel is not needed because edge pawn is already placed if possible.
    next_placement_b =
        (~pieces.down<Black>() & ~no_effect_bb.up<Black>() & no_effect_bb & no_effect_bb.down<Black>().down<Black>());
    next_placement_w = next_placement_b.up<Black>().up<Black>();
    next_count = 2 * next_placement_b.popCount();
  } while (next_count > 0 && next_count < pawn);

  if (next_count >= pawn) {
    int space = no_effect_bb.popCount() - pawn / 2 * 3;
    if (pawn % 2 == 1) {
      space -= 2;
    }
    return space >= stone;
  }

  // Case 3: 2 consecutive squares are empty.
  //   One pawns can be placed per two empty squares
  next_placement_b = (~pieces.down<Black>() & ~no_effect_bb.up<Black>() & no_effect_bb);
  next_count = next_placement_b.popCount();
  while (next_count > 0 && next_count < pawn) {
    pieces |= next_placement_b;
    no_effect_bb &= (next_placement_b & next_placement_b.up<Black>());
    pawn -= next_count;

    next_placement_b = (~pieces.down<Black>() & ~no_effect_bb.up<Black>() & no_effect_bb);
    next_count = next_placement_b.popCount();
  }

  // (#(pawn placeable square) >= pawn) and (#(empty place) >= stone)
  return (next_count >= pawn && no_effect_bb.popCount() - 2 * pawn >= stone);
}

bool GetNonDirectionalPlacement(Bitboard no_effect_bb,
                                int pawn,
                                int stone,
                                Bitboard pieces,
                                Bitboard& pawn_b,
                                Bitboard& pawn_w) {
  int empty_num = no_effect_bb.popCount();
  pawn_b = allZeroBB();
  pawn_w = allZeroBB();

  if (empty_num < pawn + stone) {
    return false;
  }

  // place pawn whose above square is effected
  Bitboard next_placement_b = ~pieces.down<Black>() & ~no_effect_bb.down<Black>() & no_effect_bb;
  Bitboard next_placement_w = ~pieces.down<White>() & ~no_effect_bb.down<White>() & ~next_placement_b & no_effect_bb;
  int next_count = next_placement_b.popCount() + next_placement_w.popCount();
  if (next_count >= pawn) {
    goto FOUND;
  }

  do {
    pawn_b |= next_placement_b;
    pawn_w |= next_placement_w;
    pieces |= next_placement_b | next_placement_w;
    no_effect_bb &=
        ~(next_placement_b | next_placement_w | next_placement_b.up<Black>() | next_placement_w.up<White>());
    pawn -= next_count;

    // centinel is not needed because edge pawn is already placed if possible.
    next_placement_b =
        (~pieces.down<Black>() & ~no_effect_bb.up<Black>() & no_effect_bb & no_effect_bb.down<Black>().down<Black>());
    next_placement_w = next_placement_b.up<Black>().up<Black>();
    next_count = 2 * next_placement_b.popCount();
  } while (next_count > 0 && next_count < pawn);

  if (next_count >= pawn) {
    int space = no_effect_bb.popCount() - pawn / 2 * 3;
    if (pawn % 2 == 1) {
      space -= 2;
    }
    if (space >= stone) {
      goto FOUND;
    } else {
      return false;
    }
  }

  next_placement_b = (~pieces.down<Black>() & ~no_effect_bb.up<Black>() & no_effect_bb);
  next_count = next_placement_b.popCount();
  while (next_count > 0 && next_count < pawn) {
    pawn_b |= next_placement_b;
    pawn_w |= next_placement_w;
    pieces |= next_placement_b;
    no_effect_bb &= (next_placement_b & next_placement_b.up<Black>());
    pawn -= next_count;

    next_placement_b = (~pieces.down<Black>() & ~no_effect_bb.up<Black>() & no_effect_bb);
    next_placement_w = allZeroBB();
    next_count = next_placement_b.popCount();
  }

  // (#(pawn placeable square) >= pawn) and (#(empty place) >= stone)
  if (next_count >= pawn && no_effect_bb.popCount() - 2 * pawn >= stone) {
    goto FOUND;
  } else {
    return false;
  }

FOUND:
  while (pawn > 0) {
    if (next_placement_b) {
      Square sq = next_placement_b.firstOneFromSQ11();
      pawn_b |= SquareMaskBB(sq);
    } else {
      Square sq = next_placement_w.firstOneFromSQ11();
      pawn_w |= SquareMaskBB(sq);
    }
    --pawn;
  }
  return true;
}
}  // namespace

namespace komori {
Search::Search(const SearchConfiguration& config) : config_{config} {
  static std::once_flag once;
  std::call_once(once, InitAttackBB);
}

int Search::Run(const PCVector& pc_list) {
  if (config_.reverse_search) {
    return RunReversible(pc_list);
  } else {
    return RunUnreversible(pc_list);
  }
}

int Search::RunUnreversible(const PCVector& pc_list) {
  // Sorting `pc_list` enables purning more effectively
  PCVector pc_list_sorted(pc_list);
  std::sort(pc_list_sorted.begin(), pc_list_sorted.end(), PCSortObject{});

  int pawn_b = CountPawnLike<Black>(pc_list_sorted);
  int pawn_w = CountPawnLike<White>(pc_list_sorted);

  PiecePositions pieces_log;
  return SearchImpl(pc_list_sorted, pawn_b, pawn_w, allOneBB(), allZeroBB(), 0, 0, pieces_log, ans_sfens_);
}

int Search::RunReversible(const PCVector& pc_list) {
  PCVector symmetry_list;
  int asymmetry_len[PieceTypeNum] = {0};
  int flip_len[PieceTypeNum] = {0};
  int pawn = 0;
  int lance = 0;
  int stone = 0;

  if (config_.all_placement) {
    throw std::runtime_error("all placement is not allowed in reversible search");
  }

  // Analyze the passed `pc_list`.
  // - pawn, stone
  // - (vertically) symmetric pieces (Rook, Bishop, Queen, King)
  // - (vertically) assymmetric pieces (other)
  for (PieceType pc : pc_list) {
    PieceType pt = SimplifyGold(Pc2Pt(pc));
    if (pt == Pawn) {
      // Lance is treated as Pawn in order to speed up
      pawn++;
    } else if (pt == Lance) {
      pawn++;
      lance++;
    } else if (pt == Stone) {
      stone++;
    } else if (IsSymmetry(pt)) {
      symmetry_list.push_back(pt);
    } else {
      asymmetry_len[pt]++;
    }
  }

  // By horizontal symmetricity, skip the first half ot search
  for (int i = PieceTypeNum - 1; i >= 0; --i) {
    if (asymmetry_len[i] > 0) {
      flip_len[i] = (asymmetry_len[i] + 1) / 2;
      if (asymmetry_len[i] % 2 == 1) {
        break;
      }
    }
  }

  // Try all conbination of piece directions
  int found_cnt = 0;
  for (;;) {
    PCVector pc_list(symmetry_list);
    for (int i = 0; i < PieceTypeNum; ++i) {
      for (int j = 0; j < flip_len[i]; ++j) {
        pc_list.push_back(PieceType(i));
      }
      for (int j = flip_len[i]; j < asymmetry_len[i]; ++j) {
        pc_list.push_back(PieceType(i | PTWhiteFlag));
      }
    }
    std::sort(pc_list.begin(), pc_list.end(), PCSortObject{});

    int search_pawn = pawn + CountPawnLikeEither(pc_list);
    int search_stone = stone + (pc_list.size() - CountPawnLikeEither(pc_list));
    PiecePositions pieces_log;
    found_cnt += SearchImplReversiblePawn(pc_list, search_pawn, search_stone, lance, allOneBB(), allZeroBB(), 0, 0,
                                          pieces_log, ans_sfens_);

    if (!config_.all_placement && found_cnt > 0) {
      goto END;
    }

    // next combination
    for (int i = 0; i < PieceTypeNum; ++i) {
      if (asymmetry_len[i] > 0) {
        // check carry-up
        if (flip_len[i] >= asymmetry_len[i]) {
          flip_len[i] = 0;
        } else {
          flip_len[i]++;
          break;
        }
      }

      // if (last combination) break;
      if (i == PieceTypeNum - 1) {
        goto END;
      }
    }
  }

END:
  std::vector<PieceType> golds;
  for (const auto& pc : pc_list) {
    auto pt = Pc2Pt(pc);
    if (pt != SimplifyGold(pt)) {
      golds.push_back(pt);
    }
  }
  auto itr = golds.cbegin();
  for (auto& sfen : ans_sfens_) {
    std::string modified_sfen;
    for (auto& c : sfen) {
      if (itr != golds.cend() && c == 'G') {
        modified_sfen += UsiString(*itr);
        itr++;
      } else if (itr != golds.cend() && c == 'g') {
        modified_sfen += UsiString(static_cast<PieceType>(*itr | PTWhiteFlag));
        itr++;
      } else {
        modified_sfen.push_back(c);
      }
    }
    sfen = std::move(modified_sfen);
  }

  return found_cnt;
}

int Search::SearchImpl(const PCVector& pc_list,
                       int pawn_b,
                       int pawn_w,
                       Bitboard no_effect_bb,
                       Bitboard pieces_bb,
                       int depth,
                       Square last_sq,
                       PiecePositions& pieces_log,
                       std::vector<std::string>& ans) {
  int pc_len = static_cast<int>(pc_list.size());

  if (depth >= pc_len) {
    // Found a placement
    ans.push_back(Pieces2Sfen(pieces_log));
    return 1;
  }

  PieceType pc = pc_list[depth];
  Bitboard pawn_allowed_b = PawnPlaceable<Black>(no_effect_bb, pieces_bb);
  Bitboard pawn_allowed_w = PawnPlaceable<White>(no_effect_bb, pieces_bb);
  Bitboard placeable_bb = no_effect_bb;
  // In order to avoid duplicate search, if a placed piece is the same as the previous one, it can be places on squares
  // that is greater than previous one.
  if (depth > 0 && pc_list[depth - 1] == pc) {
    placeable_bb &= GreaterMask(last_sq);
    if (pc == BlackPawn) {
      pawn_allowed_b &= GreaterMask(last_sq);
    } else if (pc == WhitePawn) {
      pawn_allowed_w &= GreaterMask(last_sq);
    }
  }

  int stone_b = pc_len - depth - pawn_b;
  int stonw_w = pc_len - depth - pawn_w;
  // Purning by inferier pieces method
  if (!JudgePlaceable<Black>(no_effect_bb, pawn_b, stone_b, pawn_allowed_b) ||
      !JudgePlaceable<White>(no_effect_bb, pawn_w, stonw_w, pawn_allowed_w)) {
    return 0;
  }

  int found_cnt = 0;
  while (placeable_bb.isAny()) {
    Square sq = placeable_bb.firstOneFromSQ11();
    ++node_count_;
    // Check the limit of nodes
    if (config_.node_limit != Unlimit && node_count_ >= config_.node_limit) {
      return 0;
    }

    Bitboard attack = AttackBB(pc, sq);
    if (!attack.andIsAny(pieces_bb)) {
      // Placeable pc at sq
      pieces_log.push_back({pc, sq});

      // Update #(pawn) and go to next depth
      int new_pawn_b = pawn_b - IsPawnLike<Black>(pc);
      int new_pawn_w = pawn_w - IsPawnLike<White>(pc);
      found_cnt += SearchImpl(pc_list, new_pawn_b, new_pawn_w, no_effect_bb & (~attack), pieces_bb | SquareMaskBB(sq),
                              depth + 1, sq, pieces_log, ans);
      pieces_log.pop_back();

      if (!config_.all_placement && found_cnt > 0) {
        break;
      }
    }
  }

  return found_cnt;
}

int Search::SearchImplReversiblePawn(const PCVector& pc_list,
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
    Bitboard pawn_b, pawn_w;
    // judge if remain pawns and stones are placeable
    if (GetNonDirectionalPlacement(no_effect_bb, pawn, stone, pieces_bb, pawn_b, pawn_w)) {
      if ((pawn_b & Edge2BB(Black)).popCount() + (pawn_w & Edge2BB(White)).popCount() < lance) {
        return 0;
      }
      auto stone_bb = no_effect_bb & ~pawn_b & ~pawn_w;

      PiecePositions pieces_ans(pieces_log);
      // convert pawn_bb, pawn_v_bb to pieces_log entry
      while (pawn_b.isAny()) {
        Square sq = pawn_b.firstOneFromSQ11();
        if (lance > 0 && GetRank(sq) <= 1) {
          pieces_ans.push_back({BlackLance, sq});
          lance--;
        } else {
          pieces_ans.push_back({BlackPawn, sq});
        }
      }
      while (pawn_w.isAny()) {
        Square sq = pawn_w.firstOneFromSQ11();
        if (lance > 0 && GetRank(sq) >= 7) {
          pieces_ans.push_back({WhiteLance, sq});
          lance--;
        } else {
          pieces_ans.push_back({WhitePawn, sq});
        }
      }
      while (stone_bb.isAny()) {
        Square sq = stone_bb.firstOneFromSQ11();
        if (stone > 0) {
          pieces_ans.push_back({Stone, sq});
        }
      }

      ans.push_back(Pieces2Sfen(pieces_ans));
      return 1;
    } else {
      return 0;
    }
  }

  PieceType pc = pc_list[depth];
  Bitboard placeable_bb = no_effect_bb;
  if (depth > 0 && pc_list[depth - 1] == pc) {
    placeable_bb &= GreaterMask(last_sq);
  }

  // pawn-stone purning
  if (!JudgeNonDirectionalPlacement(no_effect_bb, pawn, stone, pieces_bb)) {
    return 0;
  }

  int found_cnt = 0;
  while (placeable_bb.isAny()) {
    // check limit of nodes
    ++node_count_;
    if (config_.node_limit != Unlimit && node_count_ >= config_.node_limit) {
      return 0;
    }

    Square sq = placeable_bb.firstOneFromSQ11();
    Bitboard attack = AttackBB(pc, sq);
    if (!attack.andIsAny(pieces_bb)) {
      // placeable pc at sq
      pieces_log.push_back({pc, sq});

      int new_pawn = pawn - (IsPawnLike<Black>(pc) || IsPawnLike<White>(pc));
      int new_stone = stone - !(IsPawnLike<Black>(pc) || IsPawnLike<White>(pc));
      found_cnt += SearchImplReversiblePawn(pc_list, new_pawn, new_stone, lance, no_effect_bb & (~attack),
                                            pieces_bb | SquareMaskBB(sq), depth + 1, sq, pieces_log, ans);

      pieces_log.pop_back();

      if (!config_.all_placement && found_cnt > 0) {
        return found_cnt;
      }
    }
  }

  return found_cnt;
}

}  // namespace komori