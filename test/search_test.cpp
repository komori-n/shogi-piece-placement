#include "search.hpp"
#include "gtest/gtest.h"

class SearchTest : public ::testing::Test {};

namespace {
TEST_F(SearchTest, judge_placeable) {
  // space check
  EXPECT_TRUE(judge_placeable<false>(allOneBB(), 0, 9 * 9, allZeroBB()));
  EXPECT_FALSE(judge_placeable<false>(allOneBB(), 0, 9 * 9 + 1, allZeroBB()));

  // simple placement
  EXPECT_TRUE(judge_placeable<false>(squareMaskBB(30), 1, 0, squareMaskBB(30)));
  EXPECT_TRUE(judge_placeable<false>(squareMaskBB(30) | squareMaskBB(40), 1, 1, squareMaskBB(30) | squareMaskBB(40)));

  // complex placement
  EXPECT_TRUE(judge_placeable<false>(squareMaskBB(32) | squareMaskBB(33), 1, 0, squareMaskBB(33)));
  EXPECT_TRUE(judge_placeable<false>(squareMaskBB(32) | squareMaskBB(33) | squareMaskBB(42) | squareMaskBB(43), 1, 2,
                                     squareMaskBB(33) | squareMaskBB(43)));
  EXPECT_TRUE(judge_placeable<false>(squareMaskBB(32) | squareMaskBB(33) | squareMaskBB(42) | squareMaskBB(43), 2, 0,
                                     squareMaskBB(33) | squareMaskBB(43)));
  EXPECT_FALSE(judge_placeable<false>(squareMaskBB(32) | squareMaskBB(33) | squareMaskBB(42) | squareMaskBB(43), 1, 3,
                                      squareMaskBB(33) | squareMaskBB(43)));
  EXPECT_FALSE(judge_placeable<false>(squareMaskBB(32) | squareMaskBB(33) | squareMaskBB(42) | squareMaskBB(43), 2, 1,
                                      squareMaskBB(33) | squareMaskBB(43)));

  Bitboard bb = squareMaskBB(32) | squareMaskBB(33) | squareMaskBB(34) | squareMaskBB(35);
  EXPECT_TRUE(judge_placeable<false>(bb, 2, 0, bb & ~squareMaskBB(32)));
  EXPECT_FALSE(judge_placeable<false>(bb, 2, 1, bb & ~squareMaskBB(32)));

  // reverse
  EXPECT_TRUE(judge_placeable<true>(squareMaskBB(32) | squareMaskBB(33) | squareMaskBB(42) | squareMaskBB(43), 1, 2,
                                    squareMaskBB(32) | squareMaskBB(42)));
  EXPECT_TRUE(judge_placeable<true>(squareMaskBB(32) | squareMaskBB(33) | squareMaskBB(42) | squareMaskBB(43), 2, 0,
                                    squareMaskBB(32) | squareMaskBB(42)));
}

TEST_F(SearchTest, judge_placeable_dual) {
  Bitboard pawn_bb, pawn_v_bb;
  // space check
  EXPECT_TRUE(judge_placeable_dual(allOneBB(), 0, 9 * 9, allZeroBB()));
  EXPECT_FALSE(judge_placeable_dual(allOneBB(), 0, 9 * 9 + 1, allZeroBB()));

  // simple placement
  EXPECT_TRUE(judge_placeable_dual(squareMaskBB(32), 1, 0, allZeroBB()));
  EXPECT_TRUE(judge_placeable_dual(squareMaskBB(32) | squareMaskBB(34), 1, 0, squareMaskBB(33)));

  // 3-pawn
  EXPECT_TRUE(judge_placeable_dual(squareMaskBB(32) | squareMaskBB(33) | squareMaskBB(34), 2, 0,
                                   squareMaskBB(31) | squareMaskBB(35)));
  EXPECT_TRUE(judge_placeable_dual(
      squareMaskBB(32) | squareMaskBB(33) | squareMaskBB(34) | squareMaskBB(35) | squareMaskBB(36) | squareMaskBB(37),
      4, 0, squareMaskBB(31) | squareMaskBB(37)));
  EXPECT_TRUE(judge_placeable_dual(
      squareMaskBB(32) | squareMaskBB(33) | squareMaskBB(34) | squareMaskBB(35) | squareMaskBB(36) | squareMaskBB(37),
      3, 1, squareMaskBB(31) | squareMaskBB(38)));

  EXPECT_FALSE(judge_placeable_dual(squareMaskBB(32) | squareMaskBB(33) | squareMaskBB(34), 2, 1,
                                    squareMaskBB(31) | squareMaskBB(35)));
  EXPECT_FALSE(judge_placeable_dual(
      squareMaskBB(32) | squareMaskBB(33) | squareMaskBB(34) | squareMaskBB(35) | squareMaskBB(36) | squareMaskBB(37),
      4, 1, squareMaskBB(31) | squareMaskBB(37)));
  EXPECT_FALSE(judge_placeable_dual(
      squareMaskBB(32) | squareMaskBB(33) | squareMaskBB(34) | squareMaskBB(35) | squareMaskBB(36) | squareMaskBB(37),
      3, 2, squareMaskBB(31) | squareMaskBB(38)));

  // complex
  EXPECT_TRUE(judge_placeable_dual(squareMaskBB(33) | squareMaskBB(34), 1, 0, squareMaskBB(32) | squareMaskBB(35)));
  EXPECT_FALSE(judge_placeable_dual(squareMaskBB(33) | squareMaskBB(34), 1, 1, squareMaskBB(32) | squareMaskBB(35)));
}

TEST_F(SearchTest, Standard) {
  PCVector pc_list = {
      BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,
      BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,
      BlackPawn,   BlackPawn,   BlackLance,  BlackLance,  BlackLance,  BlackLance,  BlackKnight, BlackKnight,
      BlackKnight, BlackKnight, BlackSilver, BlackSilver, BlackSilver, BlackSilver, BlackGold,   BlackGold,
      BlackGold,   BlackGold,   BlackKing,   BlackKing,   BlackBishop, BlackBishop, BlackRook,   BlackRook,
  };

  Search search(false, Search::Unlimit);
  EXPECT_EQ(search.run(pc_list), 3720);
}

TEST_F(SearchTest, Standard_v) {
  PCVector pc_list = {
      WhitePawn,   WhitePawn,   WhitePawn,   WhitePawn,   WhitePawn,   WhitePawn,   WhitePawn,   WhitePawn,
      WhitePawn,   WhitePawn,   WhitePawn,   WhitePawn,   WhitePawn,   WhitePawn,   WhitePawn,   WhitePawn,
      WhitePawn,   WhitePawn,   WhiteLance,  WhiteLance,  WhiteLance,  WhiteLance,  WhiteKnight, WhiteKnight,
      WhiteKnight, WhiteKnight, WhiteSilver, WhiteSilver, WhiteSilver, WhiteSilver, WhiteGold,   WhiteGold,
      WhiteGold,   WhiteGold,   WhiteKing,   WhiteKing,   WhiteBishop, WhiteBishop, WhiteRook,   WhiteRook,
  };

  Search search(false, Search::Unlimit);
  EXPECT_EQ(search.run(pc_list), 3720);
}

TEST_F(SearchTest, inferior_piece) {
  PCVector pc_list = {
      WhitePawn,   WhitePawn,   WhitePawn,   WhitePawn,   WhitePawn,   WhitePawn,   WhitePawn, WhitePawn,
      WhitePawn,   WhitePawn,   WhitePawn,   WhitePawn,   WhitePawn,   WhitePawn,   WhitePawn, WhitePawn,
      WhitePawn,   WhitePawn,   WhitePawn,   WhitePawn,   WhitePawn,   WhitePawn,   WhitePawn, WhitePawn,
      WhitePawn,   WhitePawn,   WhitePawn,   WhitePawn,   WhitePawn,   WhitePawn,   WhitePawn, WhitePawn,
      WhiteKnight, WhiteKnight, WhiteKnight, WhiteKnight, WhiteBishop, WhiteBishop, WhiteRook, WhiteRook,
  };

  Search search(false, Search::Unlimit);
  EXPECT_EQ(search.run(pc_list), 26);
}

TEST_F(SearchTest, queen) {
  PCVector pc_list = {
      PieceQueen, PieceQueen, PieceQueen, PieceQueen, PieceQueen, PieceQueen, PieceQueen, PieceQueen, PieceQueen,
  };

  Search search(false, Search::Unlimit);
  EXPECT_EQ(search.run(pc_list), 352);
}

TEST_F(SearchTest, plus4pawn) {
  PCVector pc_list = {
      BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,
      BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,
      BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackLance,  BlackLance,
      BlackLance,  BlackLance,  BlackKnight, BlackKnight, BlackKnight, BlackKnight, BlackSilver, BlackSilver,
      BlackSilver, BlackSilver, BlackGold,   BlackGold,   BlackGold,   BlackGold,   BlackKing,   BlackKing,
      BlackBishop, BlackBishop, BlackRook,   BlackRook,
  };

  Search search(true, Search::Unlimit);
  EXPECT_EQ(search.run(pc_list), 1);
}

TEST_F(SearchTest, plus5pawn) {
  PCVector pc_list = {
      BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,
      BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,
      BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackPawn,   BlackLance,
      BlackLance,  BlackLance,  BlackLance,  BlackKnight, BlackKnight, BlackKnight, BlackKnight, BlackSilver,
      BlackSilver, BlackSilver, BlackSilver, BlackGold,   BlackGold,   BlackGold,   BlackGold,   BlackKing,
      BlackKing,   BlackBishop, BlackBishop, BlackRook,   BlackRook,
  };

  Search search(true, Search::Unlimit);
  EXPECT_EQ(search.run(pc_list), 0);
}
}  // namespace
