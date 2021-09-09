#ifndef KOMORI_SHOGI_HPP_
#define KOMORI_SHOGI_HPP_

#include <immintrin.h>
#include <cinttypes>
#include <iostream>
#include <string>
#include <vector>

namespace komori {
using u64 = std::uint64_t;
inline int count1s(u64 x) {
  return _mm_popcnt_u64(x);
}

inline int firstOneFromLSB(const u64 b) {
  return __builtin_ctzll(b);
}

using Square = int;
constexpr int SquareNum = (9 + 1) * 10;  // 1 is centinel row
inline Square makeSquare(int file, int rank) {
  return rank + file * 10;
}

// <pieces>
enum PieceType {
  PTWhiteFlag = 0x10,
  Stone = 0,  // 各 PieceType の or をとったもの。
  Pawn,
  Lance,
  Knight,
  Silver,
  Bishop,
  Rook,
  Gold,
  King,
  ProPawn,
  ProLance,
  ProKnight,
  ProSilver,
  ProBishop,
  ProRook,
  PieceTypeNum,

  BlackPawn = Pawn,
  BlackLance = Lance,
  BlackKnight = Knight,
  BlackSilver = Silver,
  BlackBishop = Bishop,
  BlackRook = Rook,
  BlackGold = Gold,
  BlackKing = King,
  BlackProPawn = ProPawn,
  BlackProLance = ProLance,
  BlackProKnight = ProKnight,
  BlackProSilver = ProSilver,
  BlackProBishop = ProBishop,
  BlackProRook = ProRook,

  WhitePawn = Pawn | PTWhiteFlag,
  WhiteLance = Lance | PTWhiteFlag,
  WhiteKnight = Knight | PTWhiteFlag,
  WhiteSilver = Silver | PTWhiteFlag,
  WhiteBishop = Bishop | PTWhiteFlag,
  WhiteRook = Rook | PTWhiteFlag,
  WhiteGold = Gold | PTWhiteFlag,
  WhiteKing = King | PTWhiteFlag,
  WhiteProPawn = ProPawn | PTWhiteFlag,
  WhiteProLance = ProLance | PTWhiteFlag,
  WhiteProKnight = ProKnight | PTWhiteFlag,
  WhiteProSilver = ProSilver | PTWhiteFlag,
  WhiteProBishop = ProBishop | PTWhiteFlag,
  WhiteProRook = ProRook | PTWhiteFlag,

  PieceQueen,
  PieceEmpty,

  PCNum,
};

enum Color { Black, White, ColorNum };

inline PieceType rev(PieceType pc) {
  return static_cast<PieceType>(pc ^ PTWhiteFlag);
}
inline PieceType promote(PieceType pt) {
  switch (pt) {
    case Pawn:
      return ProPawn;
    case Lance:
      return ProLance;
    case Knight:
      return ProKnight;
    case Silver:
      return ProSilver;
    case Bishop:
      return ProBishop;
    case Rook:
      return ProRook;
    default:
      return pt;
  }
}
inline PieceType simplify_gold(PieceType pt) {
  switch (pt) {
    case ProPawn:
    case ProLance:
    case ProKnight:
    case ProSilver:
      return Gold;
    default:
      return pt;
  }
}
inline bool is_symmetry(PieceType pt) {
  switch (pt) {
    case King:
    case Rook:
    case Bishop:
    case ProRook:
    case ProBishop:
    case PieceQueen:
      return true;
    default:
      return false;
  }
}
inline PieceType pc2pt(PieceType pc) {
  return pc != PieceQueen ? static_cast<PieceType>(pc & ~PTWhiteFlag) : PieceQueen;
}

struct PiecePosition {
  PieceType pc;
  Square sq;

  PiecePosition(PieceType pc, Square sq) : pc(pc), sq(sq){};
};
using PiecePositions = std::vector<PiecePosition>;
std::string pieces2sfen(const PiecePositions& pieces);

template <bool Rev>
bool is_pawn_like(PieceType pc);
// </pieces>

class Bitboard;
extern const Bitboard SquareMaskBB[SquareNum];

class Bitboard {
 public:
  Bitboard& operator=(const Bitboard& rhs) {
    _mm_store_si128(&this->m_, rhs.m_);
    return *this;
  }
  Bitboard(const Bitboard& bb) { _mm_store_si128(&this->m_, bb.m_); }
  Bitboard() {}
  Bitboard(const u64 v0, const u64 v1) {
    this->p_[0] = v0;
    this->p_[1] = v1;
  }
  u64 p(const int index) const { return p_[index]; }
  void set(const int index, const u64 val) { p_[index] = val; }
  explicit operator bool() const { return !(_mm_testz_si128(this->m_, _mm_set1_epi8(static_cast<char>(0xffu)))); }
  bool isAny() const { return static_cast<bool>(*this); }
  // これはコードが見難くなるけど仕方ない。
  bool andIsAny(const Bitboard& bb) const { return !(_mm_testz_si128(this->m_, bb.m_)); }
  Bitboard operator~() const {
    Bitboard tmp;
    _mm_store_si128(&tmp.m_, _mm_andnot_si128(this->m_, _mm_set1_epi8(static_cast<char>(0xffu))));
    return tmp;
  }
  Bitboard operator&=(const Bitboard& rhs) {
    _mm_store_si128(&this->m_, _mm_and_si128(this->m_, rhs.m_));
    return *this;
  }
  Bitboard operator|=(const Bitboard& rhs) {
    _mm_store_si128(&this->m_, _mm_or_si128(this->m_, rhs.m_));
    return *this;
  }
  Bitboard operator^=(const Bitboard& rhs) {
    _mm_store_si128(&this->m_, _mm_xor_si128(this->m_, rhs.m_));
    return *this;
  }
  Bitboard operator<<=(const int i) {
    _mm_store_si128(&this->m_, _mm_slli_epi64(this->m_, i));
    return *this;
  }
  Bitboard operator>>=(const int i) {
    _mm_store_si128(&this->m_, _mm_srli_epi64(this->m_, i));
    return *this;
  }
  Bitboard operator&(const Bitboard& rhs) const { return Bitboard(*this) &= rhs; }
  Bitboard operator|(const Bitboard& rhs) const { return Bitboard(*this) |= rhs; }
  Bitboard operator^(const Bitboard& rhs) const { return Bitboard(*this) ^= rhs; }
  Bitboard operator<<(const int i) const { return Bitboard(*this) <<= i; }
  Bitboard operator>>(const int i) const { return Bitboard(*this) >>= i; }
  bool operator==(const Bitboard& rhs) const {
    return (_mm_testc_si128(_mm_cmpeq_epi8(this->m_, rhs.m_), _mm_set1_epi8(static_cast<char>(0xffu))) ? true : false);
  }
  bool operator!=(const Bitboard& rhs) const { return !(*this == rhs); }
  // これはコードが見難くなるけど仕方ない。
  Bitboard andEqualNot(const Bitboard& bb) {
    _mm_store_si128(&this->m_, _mm_andnot_si128(bb.m_, this->m_));
    return *this;
  }
  // これはコードが見難くなるけど仕方ない。
  Bitboard notThisAnd(const Bitboard& bb) const {
    Bitboard tmp;
    _mm_store_si128(&tmp.m_, _mm_andnot_si128(this->m_, bb.m_));
    return tmp;
  }
  template <bool Rev>
  Bitboard down(void) const {
    if (!Rev) {
      return *this << 1;
    } else {
      return *this >> 1;
    }
  }
  template <bool Rev>
  Bitboard up(void) const {
    if (!Rev) {
      return *this >> 1;
    } else {
      return *this << 1;
    }
  }
  bool isSet(const Square sq) const { return andIsAny(SquareMaskBB[sq]); }
  void setBit(const Square sq) { *this |= SquareMaskBB[sq]; }
  void clearBit(const Square sq) { andEqualNot(SquareMaskBB[sq]); }
  void xorBit(const Square sq) { (*this) ^= SquareMaskBB[sq]; }
  void xorBit(const Square sq1, const Square sq2) { (*this) ^= (SquareMaskBB[sq1] | SquareMaskBB[sq2]); }
  // Bitboard の right 側だけの要素を調べて、最初に 1 であるマスの index を返す。
  // そのマスを 0 にする。
  // Bitboard の right 側が 0 でないことを前提にしている。
  Square firstOneRightFromSQ11() {
    const Square sq = static_cast<Square>(firstOneFromLSB(this->p(0)));
    // LSB 側の最初の 1 の bit を 0 にする
    this->p_[0] &= this->p(0) - 1;
    return sq;
  }
  // Bitboard の left 側だけの要素を調べて、最初に 1 であるマスの index を返す。
  // そのマスを 0 にする。
  // Bitboard の left 側が 0 でないことを前提にしている。
  Square firstOneLeftFromSQ81() {
    const Square sq = static_cast<Square>(firstOneFromLSB(this->p(1)) + 50);
    // LSB 側の最初の 1 の bit を 0 にする
    this->p_[1] &= this->p(1) - 1;
    return sq;
  }
  // Bitboard を SQ11 から SQ99 まで調べて、最初に 1 であるマスの index を返す。
  // そのマスを 0 にする。
  // Bitboard が allZeroBB() でないことを前提にしている。
  // VC++ の _BitScanForward() は入力が 0 のときに 0 を返す仕様なので、
  // 最初に 0 でないか判定するのは少し損。
  Square firstOneFromSQ11() {
    if (this->p(0))
      return firstOneRightFromSQ11();
    return firstOneLeftFromSQ81();
  }
  // 返す位置を 0 にしないバージョン。
  Square constFirstOneRightFromSQ11() const { return static_cast<Square>(firstOneFromLSB(this->p(0))); }
  Square constFirstOneLeftFromSQ81() const { return static_cast<Square>(firstOneFromLSB(this->p(1)) + 63); }
  Square constFirstOneFromSQ11() const {
    if (this->p(0))
      return constFirstOneRightFromSQ11();
    return constFirstOneLeftFromSQ81();
  }
  // Bitboard の 1 の bit を数える。
  int popCount() const { return count1s(p(0)) + count1s(p(1)); }
  // bit が 1 つだけ立っているかどうかを判定する。
  bool isOneBit() const { return this->popCount() == 1; }

  // for debug
  void printBoard() const {
    std::printf("   A  B  C  D  E  F  G  H  I\n");
    for (int r = 0; r < 10; ++r) {
      std::printf("%d", (r + 1) % 10);
      for (int f = 9 - 1; f >= 0; --f) {
        std::printf("  %c", this->isSet(makeSquare(f, r)) ? 'X' : '.');
      }
      std::putchar('\n');
    }

    std::putchar('\n');
  }

  // 指定した位置が Bitboard のどちらの u64 変数の要素か
  static int part(const Square sq) { return static_cast<int>(makeSquare(4, 9) < sq); }

 private:
  union {
    u64 p_[2];
    __m128i m_;
  };
};

extern const Bitboard AllOneBB;
extern Bitboard GreaterMaskBB[SquareNum];
extern Bitboard AttackBB[PCNum][SquareNum];
extern Bitboard EdgeBB[ColorNum];

inline Bitboard squareMaskBB(Square sq) {
  return SquareMaskBB[sq];
}
inline Bitboard greaterMask(Square sq) {
  return GreaterMaskBB[sq];
}
inline Bitboard allOneBB() {
  return AllOneBB;
}
inline Bitboard allZeroBB() {
  return Bitboard(0, 0);
}
inline Bitboard attackBB(PieceType pc, Square sq) {
  return AttackBB[pc][sq];
}
inline Bitboard edgeBB(Color c) {
  return EdgeBB[c];
}

void initAttackBB();
const char* usi_string(PieceType pc);

std::vector<PieceType> input_parse(std::string in_str);
}  // namespace komori

#endif  // KOMORI_SHOGI_HPP_