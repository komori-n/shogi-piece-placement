#include "gtest/gtest.h"
#include "shogi.hpp"

class ShogiTest : public ::testing::Test{};

namespace {
TEST_F(ShogiTest, BitOperation) {
  EXPECT_EQ(count1s(0x0000000000000000ull), 0);
  EXPECT_EQ(count1s(0x000000000000ff7full), 15);
  EXPECT_EQ(count1s(0xffffffffffffffffull), 64);

  EXPECT_EQ(firstOneFromLSB(0x8000000000000000ull), 63);
  EXPECT_EQ(firstOneFromLSB(0x0000000000000001ull), 0);
}

TEST_F(ShogiTest, PieceOperation) {
  EXPECT_EQ(rev(BlackPawn), WhitePawn);
  EXPECT_EQ(rev(WhiteRook), BlackRook);

  EXPECT_EQ(promote(Pawn), ProPawn);
  EXPECT_EQ(promote(Knight), ProKnight);

  EXPECT_EQ(simplify_gold(ProPawn), Gold);
  EXPECT_EQ(simplify_gold(Gold), Gold);
  EXPECT_EQ(simplify_gold(Lance), Lance);

  EXPECT_TRUE(is_symmetry(King));
  EXPECT_TRUE(is_symmetry(Bishop));
  EXPECT_FALSE(is_symmetry(Pawn));
  EXPECT_FALSE(is_symmetry(Silver));

  EXPECT_EQ(pc2pt(BlackSilver), Silver);
  EXPECT_EQ(pc2pt(WhiteProPawn), ProPawn);
}

TEST_F(ShogiTest, IsPawnLike) {
  // pawn-like pieces
  EXPECT_TRUE(is_pawn_like<false>(BlackPawn));
  EXPECT_TRUE(is_pawn_like<false>(BlackLance));
  EXPECT_TRUE(is_pawn_like<false>(BlackSilver));
  EXPECT_TRUE(is_pawn_like<false>(BlackGold));
  EXPECT_TRUE(is_pawn_like<false>(BlackProPawn));
  EXPECT_TRUE(is_pawn_like<false>(WhiteGold));

  // reverse pawn-like pieces
  EXPECT_TRUE(is_pawn_like<true>(WhitePawn));
  EXPECT_TRUE(is_pawn_like<true>(WhiteSilver));

  // not pawn-like pieces
  EXPECT_FALSE(is_pawn_like<false>(BlackKnight));
  EXPECT_FALSE(is_pawn_like<false>(WhitePawn));

  // not reverse pawn-like pieces
  EXPECT_FALSE(is_pawn_like<true>(BlackKnight));
  EXPECT_FALSE(is_pawn_like<true>(BlackSilver));
  EXPECT_FALSE(is_pawn_like<true>(WhiteBishop));
}
}