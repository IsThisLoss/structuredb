#pragma once

#include <chrono>

#include <boost/asio/steady_timer.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>

#include "types.hpp"

namespace structuredb::server::io {

/// Suspend the current coroutine without blocking its executor.
inline Awaitable<void> Delay(std::chrono::steady_clock::duration duration) {
  auto executor = co_await boost::asio::this_coro::executor;
  boost::asio::steady_timer timer{executor, duration};
  co_await timer.async_wait(boost::asio::use_awaitable);
}

}
