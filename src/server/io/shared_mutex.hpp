#pragma once

#include <boost/asio/io_context.hpp>

#include "types.hpp"

namespace structuredb::server::io {

/// @brief implements shared lock
class SharedMutex {
public:
  explicit SharedMutex(boost::asio::io_context& io_context);

  /// @brief locks mutext in shared mode
  Awaitable<void> LockShared();

  /// @brief unlocks mutex
  Awaitable<void> UnlockShared();

  /// @brief locks mutex in exclusive mode
  Awaitable<void> LockExclusive();

  /// @brief unlocks mutex
  Awaitable<void> UnlockExclusive();

private:
  boost::asio::io_context& io_context_;
  int state_{0}; // -1 locked exclusive otherwise the number of shared locks
  std::deque<std::function<void()>> waiters_;


  Awaitable<void> TryLock(std::function<bool()> predicate);

  Awaitable<void> NotifyWaiters();
};

}
