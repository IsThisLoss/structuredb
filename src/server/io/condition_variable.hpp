#pragma once

#include <boost/asio/io_context.hpp>

#include <deque>
#include <functional>

#include "types.hpp"

namespace structuredb::server::io {

/// @brief coroutine-friendly condition variable for a single-threaded io_context
///
/// Unlike std::condition_variable it does not block the event loop thread:
/// a waiting coroutine is suspended while other coroutines keep running on the
/// same thread. Modelled after the waiter mechanism used in SharedMutex.
class ConditionVariable {
public:
  explicit ConditionVariable(boost::asio::io_context& io_context);

  /// @brief suspends the calling coroutine until @p predicate becomes true
  ///
  /// The predicate is re-checked every time the coroutine is woken via NotifyAll.
  /// Relies on cooperative scheduling: there is no preemption between the moment
  /// the predicate returns true and the moment the caller acts on it.
  Awaitable<void> Wait(const std::function<bool()>& predicate);

  /// @brief wakes every waiting coroutine so each can re-check its predicate
  void NotifyAll();

private:
  boost::asio::io_context& io_context_;
  std::deque<std::function<void()>> waiters_;
};

}
