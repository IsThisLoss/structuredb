#pragma once

#include <gtest/gtest.h>

#define CO_ASSERT_TRUE(condition) \
  EXPECT_TRUE(condition); \
  co_return

#define CO_ASSERT_FALSE(condition) \
  EXPECT_FALSE(condition); \
  co_return

#define CO_ASSERT_EQ(val1, val2) \
  EXPECT_PRED_FORMAT2(::testing::internal::EqHelper::Compare, val1, val2); \
  co_return

#define CO_ASSERT_NE(val1, val2) \
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperNE, val1, val2); \
  co_return

#define CO_ASSERT_LE(val1, val2) \
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperLE, val1, val2); \
  co_return

#define CO_ASSERT_LT(val1, val2) \
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperLT, val1, val2); \
  co_return

#define CO_ASSERT_GE(val1, val2) \
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperGE, val1, val2); \
  co_return
  
#define CO_ASSERT_GT(val1, val2) \
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperGT, val1, val2); \
  co_return