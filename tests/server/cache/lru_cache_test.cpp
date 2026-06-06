#include <gtest/gtest.h>

#include <string>

#include <cache/lru_cache.hpp>

namespace structuredb::tests {

using structuredb::server::cache::LruCache;

TEST(LruCacheTest, MissOnEmpty) {
  LruCache<int, std::string> cache{2};
  EXPECT_EQ(cache.Get(1), nullptr);
  EXPECT_EQ(cache.Size(), 0u);
}

TEST(LruCacheTest, PutThenGet) {
  LruCache<int, std::string> cache{2};
  cache.Put(1, "one");

  auto* value = cache.Get(1);
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(*value, "one");
  EXPECT_EQ(cache.Size(), 1u);
}

TEST(LruCacheTest, PutOverwritesExisting) {
  LruCache<int, std::string> cache{2};
  cache.Put(1, "one");
  cache.Put(1, "ONE");

  auto* value = cache.Get(1);
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(*value, "ONE");
  EXPECT_EQ(cache.Size(), 1u);
}

TEST(LruCacheTest, EvictsLeastRecentlyUsed) {
  LruCache<int, std::string> cache{2};
  cache.Put(1, "one");
  cache.Put(2, "two");
  cache.Put(3, "three");  // evicts key 1 (least-recently-used)

  EXPECT_EQ(cache.Get(1), nullptr);
  ASSERT_NE(cache.Get(2), nullptr);
  ASSERT_NE(cache.Get(3), nullptr);
  EXPECT_EQ(cache.Size(), 2u);
}

TEST(LruCacheTest, GetRefreshesRecency) {
  LruCache<int, std::string> cache{2};
  cache.Put(1, "one");
  cache.Put(2, "two");

  // touch key 1 so key 2 becomes least-recently-used
  ASSERT_NE(cache.Get(1), nullptr);

  cache.Put(3, "three");  // evicts key 2, not key 1

  ASSERT_NE(cache.Get(1), nullptr);
  EXPECT_EQ(cache.Get(2), nullptr);
  ASSERT_NE(cache.Get(3), nullptr);
}

TEST(LruCacheTest, OverwriteRefreshesRecency) {
  LruCache<int, std::string> cache{2};
  cache.Put(1, "one");
  cache.Put(2, "two");
  cache.Put(1, "one-again");  // refreshes key 1
  cache.Put(3, "three");      // evicts key 2

  ASSERT_NE(cache.Get(1), nullptr);
  EXPECT_EQ(cache.Get(2), nullptr);
  ASSERT_NE(cache.Get(3), nullptr);
}

TEST(LruCacheTest, ZeroCapacityCachesNothing) {
  LruCache<int, std::string> cache{0};
  cache.Put(1, "one");

  EXPECT_EQ(cache.Get(1), nullptr);
  EXPECT_EQ(cache.Size(), 0u);
  EXPECT_EQ(cache.Capacity(), 0u);
}

TEST(LruCacheTest, ContainsDoesNotChangeRecency) {
  LruCache<int, std::string> cache{2};
  cache.Put(1, "one");
  cache.Put(2, "two");

  // Contains must not refresh key 1, so it stays the eviction victim
  EXPECT_TRUE(cache.Contains(1));
  cache.Put(3, "three");

  EXPECT_FALSE(cache.Contains(1));
  EXPECT_TRUE(cache.Contains(2));
  EXPECT_TRUE(cache.Contains(3));
}

TEST(LruCacheTest, Clear) {
  LruCache<int, std::string> cache{2};
  cache.Put(1, "one");
  cache.Put(2, "two");
  cache.Clear();

  EXPECT_EQ(cache.Size(), 0u);
  EXPECT_EQ(cache.Get(1), nullptr);
  EXPECT_EQ(cache.Get(2), nullptr);
}

}
