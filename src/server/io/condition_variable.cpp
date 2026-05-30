#include "condition_variable.hpp"

#include <functional>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/error.hpp>

namespace structuredb::server::io {

ConditionVariable::ConditionVariable(boost::asio::io_context& io_context)
  : io_context_{io_context}
{}

Awaitable<void> ConditionVariable::Wait(const std::function<bool()>& predicate) {
  while (!predicate()) {
    boost::asio::steady_timer timer{io_context_};
    timer.expires_at(boost::asio::steady_timer::time_point::max());
    waiters_.emplace_back([&timer]() { timer.cancel(); });
    try {
      co_await timer.async_wait(boost::asio::use_awaitable);
    } catch (const boost::system::system_error& e) {
      if (e.code() != boost::asio::error::operation_aborted) {
        throw;
      }
    }
  }
}

void ConditionVariable::NotifyAll() {
  // Cancelling a waiter's timer schedules its coroutine for resumption on the
  // io_context; it does not run inline, so this is safe to call synchronously.
  while (!waiters_.empty()) {
    auto handler = std::move(waiters_.front());
    waiters_.pop_front();
    handler();
  }
}

}
