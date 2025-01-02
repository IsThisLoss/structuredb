#include "shared_mutex.hpp"

#include <boost/asio/steady_timer.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/error.hpp>

namespace structuredb::server::io {

SharedMutex::SharedMutex(boost::asio::io_context& io_context)
  : io_context_{io_context}
{}

Awaitable<void> SharedMutex::LockShared() {
  co_await TryLock([this]() { return state_ >= 0; });
  state_++;
}

Awaitable<void> SharedMutex::UnlockShared() {
  state_--;
  co_await NotifyWaiters();
}

Awaitable<void> SharedMutex::LockExclusive() {
  co_await TryLock([this]() { return state_ == 0; });
  state_ = -1;
}

Awaitable<void> SharedMutex::UnlockExclusive() {
  state_ = 0;
  co_await NotifyWaiters();
}

Awaitable<void> SharedMutex::TryLock(std::function<bool()> predicate) {
  while (!predicate()) {
    boost::asio::steady_timer timer{io_context_};
    timer.expires_at(boost::asio::steady_timer::time_point::max());
    waiters_.emplace_back([&timer]() { timer.cancel(); });
    try {
        co_await timer.async_wait(boost::asio::use_awaitable);
    } catch (const boost::system::system_error& e) {
        if (e.code() !=  boost::asio::error::operation_aborted) {
          throw;
        }
    }
  }
}

Awaitable<void> SharedMutex::NotifyWaiters() {
  while (!waiters_.empty()) {
      auto handler = std::move(waiters_.front());
      waiters_.pop_front();
      handler();
  }
  co_return;
}

}
