#include <gtest/gtest.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/use_future.hpp>

#include <replication/follower_registry.hpp>

namespace structuredb::tests {

TEST(FollowerRegistryTest, InterleavedFollowersRetainOldestSegment) {
  boost::asio::io_context context;
  server::replication::FollowerRegistry registry;
  auto first = boost::asio::co_spawn(context, [&]() -> server::Awaitable<void> {
    auto token = registry.Register({.segment_no = 2, .page_no = 0});
    co_await boost::asio::post(context, boost::asio::use_awaitable);
    EXPECT_EQ(registry.MinRetainedSegment(), 2);
    registry.Update(token, {.segment_no = 7, .page_no = 0});
    co_await boost::asio::post(context, boost::asio::use_awaitable);
    EXPECT_EQ(registry.MinRetainedSegment(), 5);
    registry.Unregister(token);
  }, boost::asio::use_future);
  auto second = boost::asio::co_spawn(context, [&]() -> server::Awaitable<void> {
    auto token = registry.Register({.segment_no = 5, .page_no = 0});
    co_await boost::asio::post(context, boost::asio::use_awaitable);
    co_await boost::asio::post(context, boost::asio::use_awaitable);
    EXPECT_EQ(registry.MinRetainedSegment(), 5);
    registry.Unregister(token);
  }, boost::asio::use_future);
  context.run();
  first.get();
  second.get();
  EXPECT_FALSE(registry.MinRetainedSegment().has_value());
}

}
