#include <algorithm>
#include <map>
#include <sstream>

#include "shogi.hpp"

namespace {
constexpr bool kIsPawnLike[komori::PCNum] = {
    false,                       // BStone
    true,  true,  false, true,   // BPawn, BLance, BKnight, BSilver
    false, true,  true,  true,   // BBishop, BRook, BGold, BKing
    true,  true,  true,  true,   // BProGolds
    true,  true,                 // BProBishop, BProRook
    false,                       // None
    false,                       // WStone
    false, false, false, false,  // WPawn, WLance, WKnight, WSilver
    false, true,  true,  true,   // WBishop, WRook, WGold, WKing
    true,  true,  true,  true,   // WProGolds
    true,  true,                 // BProBishop, BProRook
    true,                        // Queen
    false                        // None
};
}  // namespace

namespace komori {
const Bitboard kSquareMaskBB[SquareNum] = {
    Bitboard(u64(1) << 0, 0),  Bitboard(u64(1) << 1, 0),  Bitboard(u64(1) << 2, 0),  Bitboard(u64(1) << 3, 0),
    Bitboard(u64(1) << 4, 0),  Bitboard(u64(1) << 5, 0),  Bitboard(u64(1) << 6, 0),  Bitboard(u64(1) << 7, 0),
    Bitboard(u64(1) << 8, 0),  Bitboard(u64(1) << 9, 0),

    Bitboard(u64(1) << 10, 0), Bitboard(u64(1) << 11, 0), Bitboard(u64(1) << 12, 0), Bitboard(u64(1) << 13, 0),
    Bitboard(u64(1) << 14, 0), Bitboard(u64(1) << 15, 0), Bitboard(u64(1) << 16, 0), Bitboard(u64(1) << 17, 0),
    Bitboard(u64(1) << 18, 0), Bitboard(u64(1) << 19, 0),

    Bitboard(u64(1) << 20, 0), Bitboard(u64(1) << 21, 0), Bitboard(u64(1) << 22, 0), Bitboard(u64(1) << 23, 0),
    Bitboard(u64(1) << 24, 0), Bitboard(u64(1) << 25, 0), Bitboard(u64(1) << 26, 0), Bitboard(u64(1) << 27, 0),
    Bitboard(u64(1) << 28, 0), Bitboard(u64(1) << 29, 0),

    Bitboard(u64(1) << 30, 0), Bitboard(u64(1) << 31, 0), Bitboard(u64(1) << 32, 0), Bitboard(u64(1) << 33, 0),
    Bitboard(u64(1) << 34, 0), Bitboard(u64(1) << 35, 0), Bitboard(u64(1) << 36, 0), Bitboard(u64(1) << 37, 0),
    Bitboard(u64(1) << 38, 0), Bitboard(u64(1) << 39, 0),

    Bitboard(u64(1) << 40, 0), Bitboard(u64(1) << 41, 0), Bitboard(u64(1) << 42, 0), Bitboard(u64(1) << 43, 0),
    Bitboard(u64(1) << 44, 0), Bitboard(u64(1) << 45, 0), Bitboard(u64(1) << 46, 0), Bitboard(u64(1) << 47, 0),
    Bitboard(u64(1) << 48, 0), Bitboard(u64(1) << 49, 0),

    Bitboard(0, u64(1) << 0),  Bitboard(0, u64(1) << 1),  Bitboard(0, u64(1) << 2),  Bitboard(0, u64(1) << 3),
    Bitboard(0, u64(1) << 4),  Bitboard(0, u64(1) << 5),  Bitboard(0, u64(1) << 6),  Bitboard(0, u64(1) << 7),
    Bitboard(0, u64(1) << 8),  Bitboard(0, u64(1) << 9),

    Bitboard(0, u64(1) << 10), Bitboard(0, u64(1) << 11), Bitboard(0, u64(1) << 12), Bitboard(0, u64(1) << 13),
    Bitboard(0, u64(1) << 14), Bitboard(0, u64(1) << 15), Bitboard(0, u64(1) << 16), Bitboard(0, u64(1) << 17),
    Bitboard(0, u64(1) << 18), Bitboard(0, u64(1) << 19),

    Bitboard(0, u64(1) << 20), Bitboard(0, u64(1) << 21), Bitboard(0, u64(1) << 22), Bitboard(0, u64(1) << 23),
    Bitboard(0, u64(1) << 24), Bitboard(0, u64(1) << 25), Bitboard(0, u64(1) << 26), Bitboard(0, u64(1) << 27),
    Bitboard(0, u64(1) << 28), Bitboard(0, u64(1) << 29),

    Bitboard(0, u64(1) << 30), Bitboard(0, u64(1) << 31), Bitboard(0, u64(1) << 32), Bitboard(0, u64(1) << 33),
    Bitboard(0, u64(1) << 34), Bitboard(0, u64(1) << 35), Bitboard(0, u64(1) << 36), Bitboard(0, u64(1) << 37),
    Bitboard(0, u64(1) << 38), Bitboard(0, u64(1) << 39),
};

const Bitboard AllOneBB =
    (Bitboard(u64(0x1ff) << 0, 0) | Bitboard(u64(0x1ff) << 10, 0) | Bitboard(u64(0x1ff) << 20, 0) |
     Bitboard(u64(0x1ff) << 30, 0) | Bitboard(u64(0x1ff) << 40, 0) | Bitboard(0, u64(0x1ff) << 0) |
     Bitboard(0, u64(0x1ff) << 10) | Bitboard(0, u64(0x1ff) << 20) | Bitboard(0, u64(0x1ff) << 30));

Bitboard GreaterMaskBB[SquareNum];
Bitboard EdgeBB[ColorNum];

namespace {
bool isInSquare(int file, int rank) {
  return file >= 0 && file < 9 && rank >= 0 && rank < 9;
}

bool is_initialized = false;
}  // namespace

Bitboard kAttackBB[PCNum][SquareNum];

void InitAttackBB() {
  int dr[PCNum][ColorNum][8] = {
      // Stone
      {{0}, {0}},
      // Pawn
      {{-1}, {1}},
      // Lance
      {{-1}, {1}},
      // Knight
      {{-2, -2}, {2, 2}},
      // Silver
      {{-1, -1, -1, 1, 1}, {1, 1, 1, -1, -1}},
      // Bishop
      {{-1, -1, 1, 1}, {-1, -1, 1, 1}},
      // Rook
      {{-1, 1, 0, 0}, {-1, 1, 0, 0}},
      // Gold
      {{-1, -1, -1, 0, 0, 1}, {1, 1, 1, 0, 0, -1}},
      // King
      {{-1, -1, -1, 0, 0, 1, 1, 1}, {1, 1, 1, 0, 0, -1, -1, -1}},
  };

  int df[PCNum][ColorNum][8] = {
      // Stone
      {{0}, {0}},
      // Pawn
      {{0}, {0}},
      // Lance
      {{0}, {0}},
      // Knight
      {{-1, 1}, {-1, 1}},
      // Silver
      {{-1, 0, 1, -1, 1}, {-1, 0, 1, -1, 1}},
      // Bishop
      {{-1, 1, -1, 1}, {-1, 1, -1, 1}},
      // Rook
      {{0, 0, -1, 1}, {0, 0, -1, 1}},
      // Gold
      {{-1, 0, 1, -1, 1, 0}, {-1, 0, 1, -1, 1, 0}},
      // King
      {{-1, 0, 1, -1, 1, -1, 0, 1}, {-1, 0, 1, -1, 1, -1, 0, 1}},
  };

  if (is_initialized) {
    return;
  }
  is_initialized = true;

  for (int pt = 0; pt < PieceTypeNum; ++pt) {
    for (int color = 0; color < ColorNum; ++color) {
      for (int r = 0; r < 9; ++r) {
        for (int f = 0; f < 9; ++f) {
          int sq = MakeSquare(f, r);
          int pc = pt | (color == Black ? 0 : PTWhiteFlag);
          kAttackBB[pc][sq] = SquareMaskBB(sq);
          for (int i = 0; i < 8; ++i) {
            if (dr[pt][color][i] == 0 && df[pt][color][i] == 0)
              break;
            int ri = r + dr[pt][color][i];
            int fi = f + df[pt][color][i];

            while (isInSquare(fi, ri)) {
              kAttackBB[pc][sq] |= SquareMaskBB(MakeSquare(fi, ri));
              if (!(pt == Lance || pt == Rook || pt == Bishop))
                break;

              ri += dr[pt][color][i];
              fi += df[pt][color][i];
            }
          }
        }
      }
    }
  }
  for (int r = 0; r < 9; ++r) {
    for (int f = 0; f < 9; ++f) {
      Square sq = MakeSquare(f, r);
      kAttackBB[BlackProBishop][sq] = kAttackBB[BlackBishop][sq] | kAttackBB[BlackKing][sq];
      kAttackBB[BlackProRook][sq] = kAttackBB[BlackRook][sq] | kAttackBB[BlackKing][sq];
      kAttackBB[WhiteProBishop][sq] = kAttackBB[WhiteBishop][sq] | kAttackBB[WhiteKing][sq];
      kAttackBB[WhiteProRook][sq] = kAttackBB[WhiteRook][sq] | kAttackBB[WhiteKing][sq];
      kAttackBB[PieceQueen][sq] = kAttackBB[BlackBishop][sq] | kAttackBB[BlackRook][sq];
    }
  }

  for (Square sq = 0; sq < SquareNum; ++sq) {
    GreaterMaskBB[sq] = AllOneBB;
    for (Square sq2 = 0; sq2 <= sq; ++sq2) {
      GreaterMaskBB[sq] ^= kSquareMaskBB[sq2];
    }
  }

  for (int f = 0; f < 9; ++f) {
    EdgeBB[Black] |= kSquareMaskBB[MakeSquare(f, 0)];
    EdgeBB[White] |= kSquareMaskBB[MakeSquare(f, 8)];
  }
}

const char* UsiString(PieceType pc) {
  const char* usi_table[PCNum] = {"X",  "P",  "L",  "N",  "S",  "B",  "R",  "G",  "K",  "+P", "+L",
                                  "+N", "+S", "+B", "+R", "E",  "X",  "p",  "l",  "n",  "s",  "b",
                                  "r",  "g",  "k",  "+p", "+l", "+n", "+s", "+b", "+r", "Q",  "E"};

  return usi_table[pc];
}

std::string Pieces2Sfen(const PiecePositions& pieces) {
  std::ostringstream ss;
  int space = 0;
  PieceType pcs[SquareNum];
  std::fill(pcs, pcs + SquareNum, PieceEmpty);

  for (auto piece : pieces) {
    pcs[piece.sq] = piece.pc;
  }

  for (int r = 0; r < 9; ++r) {
    for (int f = 0; f < 9; ++f) {
      int sq = MakeSquare(f, r);
      if (pcs[sq] == PieceEmpty) {
        ++space;
      } else {
        if (space) {
          ss << space;
          space = 0;
        }
        ss << UsiString(pcs[sq]);
      }
    }
    if (space) {
      ss << space;
      space = 0;
    }
    if (r != 8)
      ss << "/";
  }
  ss << " b - 1";
  return ss.str();
}

template <Color C>
bool IsPawnLike(PieceType pc) {
  if constexpr (C == White) {
    pc = Reverse(pc);
  }

  return kIsPawnLike[pc];
}

std::vector<PieceType> InputParse(std::string in_str) {
  std::vector<PieceType> piecetypes;
  std::map<char, PieceType> text2pt;

  text2pt['X'] = Stone;
  text2pt['P'] = BlackPawn;
  text2pt['L'] = BlackLance;
  text2pt['N'] = BlackKnight;
  text2pt['S'] = BlackSilver;
  text2pt['G'] = BlackGold;
  text2pt['K'] = BlackKing;
  text2pt['R'] = BlackRook;
  text2pt['B'] = BlackBishop;

  text2pt['p'] = WhitePawn;
  text2pt['l'] = WhiteLance;
  text2pt['n'] = WhiteKnight;
  text2pt['s'] = WhiteSilver;
  text2pt['g'] = WhiteGold;
  text2pt['k'] = WhiteKing;
  text2pt['r'] = WhiteRook;
  text2pt['b'] = WhiteBishop;

  text2pt['q'] = PieceQueen;
  text2pt['Q'] = PieceQueen;

  PieceType pt = PieceEmpty;
  bool promote_flag = false;
  int number_buf = 0;
  for (char c : in_str) {
    if (!std::isdigit(c) && pt != PieceEmpty) {
      if (number_buf == 0) {
        number_buf = 1;
      }

      PieceType pc = promote_flag ? Promote(pt) : pt;
      for (int i = 0; i < number_buf; ++i) {
        piecetypes.push_back(pc);
      }
      pt = PieceEmpty;
      promote_flag = false;
    }

    if (std::isdigit(c)) {
      number_buf = number_buf * 10 + static_cast<int>(c - '0');
    } else if (text2pt.find(c) != text2pt.end()) {
      if (pt != PieceEmpty) {
      }

      number_buf = 0;
      pt = text2pt[c];
    } else if (c == '+') {
      promote_flag = true;
    }
  }

  if (pt != PieceEmpty) {
    if (number_buf == 0) {
      number_buf = 1;
    }

    PieceType pc = promote_flag ? Promote(pt) : pt;
    for (int i = 0; i < number_buf; ++i) {
      piecetypes.push_back(pc);
    }
  }

  return piecetypes;
}

// explicit instanciation
template bool IsPawnLike<Black>(PieceType pc);
template bool IsPawnLike<White>(PieceType pc);
}  // namespace komori