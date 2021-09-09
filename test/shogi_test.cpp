#include "shogi.hpp"
#include "gtest/gtest.h"

class ShogiTest : public ::testing::Test {};

namespace {
TEST_F(ShogiTest, BitOperation) {
  EXPECT_EQ(Count1s(0x0000000000000000ull), 0);
  EXPECT_EQ(Count1s(0x000000000000ff7full), 15);
  EXPECT_EQ(Count1s(0xffffffffffffffffull), 64);

  EXPECT_EQ(FirstOneFromLSB(0x8000000000000000ull), 63);
  EXPECT_EQ(FirstOneFromLSB(0x0000000000000001ull), 0);
}

TEST_F(ShogiTest, PieceOperation) {
  EXPECT_EQ(Reverse(BlackPawn), WhitePawn);
  EXPECT_EQ(Reverse(WhiteRook), BlackRook);

  EXPECT_EQ(Promote(Pawn), ProPawn);
  EXPECT_EQ(Promote(Knight), ProKnight);

  EXPECT_EQ(SimplifyGold(ProPawn), Gold);
  EXPECT_EQ(SimplifyGold(Gold), Gold);
  EXPECT_EQ(SimplifyGold(Lance), Lance);

  EXPECT_TRUE(IsSymmetry(King));
  EXPECT_TRUE(IsSymmetry(Bishop));
  EXPECT_FALSE(IsSymmetry(Pawn));
  EXPECT_FALSE(IsSymmetry(Silver));

  EXPECT_EQ(Pc2Pt(BlackSilver), Silver);
  EXPECT_EQ(Pc2Pt(WhiteProPawn), ProPawn);
}

TEST_F(ShogiTest, IsPawnLike) {
  // pawn-like pieces
  EXPECT_TRUE(IsPawnLike<false>(BlackPawn));
  EXPECT_TRUE(IsPawnLike<false>(BlackLance));
  EXPECT_TRUE(IsPawnLike<false>(BlackSilver));
  EXPECT_TRUE(IsPawnLike<false>(BlackGold));
  EXPECT_TRUE(IsPawnLike<false>(BlackProPawn));
  EXPECT_TRUE(IsPawnLike<false>(WhiteGold));

  // reverse pawn-like pieces
  EXPECT_TRUE(IsPawnLike<true>(WhitePawn));
  EXPECT_TRUE(IsPawnLike<true>(WhiteSilver));

  // not pawn-like pieces
  EXPECT_FALSE(IsPawnLike<false>(BlackKnight));
  EXPECT_FALSE(IsPawnLike<false>(WhitePawn));

  // not reverse pawn-like pieces
  EXPECT_FALSE(IsPawnLike<true>(BlackKnight));
  EXPECT_FALSE(IsPawnLike<true>(BlackSilver));
  EXPECT_FALSE(IsPawnLike<true>(WhiteBishop));
}
}  // namespace