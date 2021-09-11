#ifndef KOMORI_SHOGI_HPP_
#define KOMORI_SHOGI_HPP_

#include <immintrin.h>
#include <cinttypes>
#include <string>
#include <vector>

namespace komori {
using u64 = std::uint64_t;
/// Square in the board
using Square = int;
/// Row==0 is a dummy row, which simplify the calculation of effects of edge pawns
constexpr int SquareNum = (9 + 1) * 10;

/// Count the number of 1s
inline int Count1s(u64 x) {
  return _mm_popcnt_u64(x);
}

/// Get the least significant bit (LSB)
inline int FirstOneFromLSB(const u64 b) {
  return __builtin_ctzll(b);
}

/// Calculate square from a file and a rank
inline Square MakeSquare(int file, int rank) {
  return rank + file * 10;
}

inline int GetRank(Square sq) {
  return sq % 10;
}

// <pieces>
/**
 * @brief A type of pieces.
 */
enum PieceType {
  PTWhiteFlag = 0x10,
  Stone = 0,
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

  PCNum,  ///< The number of PieceType
};

/// Players
enum Color { Black, White, ColorNum };

/// Flip the color
inline PieceType Reverse(PieceType pc) {
  return static_cast<PieceType>(pc ^ PTWhiteFlag);
}

/// Promote a piece if possible
inline PieceType Promote(PieceType pt) {
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

/// Replace gold-like pieces (like promoted pawns) into Gold
inline PieceType SimplifyGold(PieceType pt) {
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

/// Judge if the effect of `pt` is vertically symmetric
inline bool IsSymmetry(PieceType pt) {
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

/// Get the piece type (without color information)
inline PieceType Pc2Pt(PieceType pc) {
  return pc != PieceQueen ? static_cast<PieceType>(pc & ~PTWhiteFlag) : PieceQueen;
}

/// A pair of a piece and a square
struct PiecePosition {
  PieceType pc{Stone};
  Square sq{0};
};
/// A vector of `PiecePosition`
using PiecePositions = std::vector<PiecePosition>;
/// Get a string representing the board
std::string Pieces2Sfen(const PiecePositions& pieces);

/// Judge if `pc` has an effect on the forwarding square
template <Color C>
bool IsPawnLike(PieceType pc);
// </pieces>

class Bitboard;
extern const Bitboard kSquareMaskBB[SquareNum];

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
  template <Color C>
  Bitboard down(void) const {
    if constexpr (C == Black) {
      return *this << 1;
    } else {
      return *this >> 1;
    }
  }
  template <Color C>
  Bitboard up(void) const {
    if constexpr (C == Black) {
      return *this >> 1;
    } else {
      return *this << 1;
    }
  }
  bool isSet(const Square sq) const { return andIsAny(kSquareMaskBB[sq]); }
  void setBit(const Square sq) { *this |= kSquareMaskBB[sq]; }
  void clearBit(const Square sq) { andEqualNot(kSquareMaskBB[sq]); }
  void xorBit(const Square sq) { (*this) ^= kSquareMaskBB[sq]; }
  void xorBit(const Square sq1, const Square sq2) { (*this) ^= (kSquareMaskBB[sq1] | kSquareMaskBB[sq2]); }
  // Bitboard の right 側だけの要素を調べて、最初に 1 であるマスの index を返す。
  // そのマスを 0 にする。
  // Bitboard の right 側が 0 でないことを前提にしている。
  Square firstOneRightFromSQ11() {
    const Square sq = static_cast<Square>(FirstOneFromLSB(this->p(0)));
    // LSB 側の最初の 1 の bit を 0 にする
    this->p_[0] &= this->p(0) - 1;
    return sq;
  }
  // Bitboard の left 側だけの要素を調べて、最初に 1 であるマスの index を返す。
  // そのマスを 0 にする。
  // Bitboard の left 側が 0 でないことを前提にしている。
  Square firstOneLeftFromSQ81() {
    const Square sq = static_cast<Square>(FirstOneFromLSB(this->p(1)) + 50);
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
  Square constFirstOneRightFromSQ11() const { return static_cast<Square>(FirstOneFromLSB(this->p(0))); }
  Square constFirstOneLeftFromSQ81() const { return static_cast<Square>(FirstOneFromLSB(this->p(1)) + 63); }
  Square constFirstOneFromSQ11() const {
    if (this->p(0))
      return constFirstOneRightFromSQ11();
    return constFirstOneLeftFromSQ81();
  }
  // Bitboard の 1 の bit を数える。
  int popCount() const { return Count1s(p(0)) + Count1s(p(1)); }
  // bit が 1 つだけ立っているかどうかを判定する。
  bool isOneBit() const { return this->popCount() == 1; }

  // for debug
  void printBoard() const {
    std::printf("   A  B  C  D  E  F  G  H  I\n");
    for (int r = 0; r < 10; ++r) {
      std::printf("%d", (r + 1) % 10);
      for (int f = 9 - 1; f >= 0; --f) {
        std::printf("  %c", this->isSet(MakeSquare(f, r)) ? 'X' : '.');
      }
      std::putchar('\n');
    }

    std::putchar('\n');
  }

  // 指定した位置が Bitboard のどちらの u64 変数の要素か
  static int part(const Square sq) { return static_cast<int>(MakeSquare(4, 9) < sq); }

 private:
  union {
    u64 p_[2];
    __m128i m_;
  };
};

extern const Bitboard AllOneBB;
extern Bitboard kGreaterMaskBB[SquareNum];
extern Bitboard kAttackBB[PCNum][SquareNum];
extern Bitboard kEdge2BB[ColorNum];

inline Bitboard SquareMaskBB(Square sq) {
  return kSquareMaskBB[sq];
}
inline Bitboard GreaterMask(Square sq) {
  return kGreaterMaskBB[sq];
}
inline Bitboard allOneBB() {
  return AllOneBB;
}
inline Bitboard allZeroBB() {
  return Bitboard(0, 0);
}
inline Bitboard AttackBB(PieceType pc, Square sq) {
  return kAttackBB[pc][sq];
}
inline Bitboard Edge2BB(Color c) {
  return kEdge2BB[c];
}

void InitAttackBB();
const char* UsiString(PieceType pc);

std::vector<PieceType> InputParse(std::string in_str);
}  // namespace komori

#endif  // KOMORI_SHOGI_HPP_
