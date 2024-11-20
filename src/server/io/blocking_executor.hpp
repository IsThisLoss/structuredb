#pragma once

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/use_awaitable.hpp>

#include "types.hpp"

namespace structuredb::server::io {

class BlockingExecutor {
public:
  explicit BlockingExecutor(size_t threads_num);

  template <std::invocable<> Func>
  Awaitable<std::invoke_result_t<Func>> Execute(Func&& func) {
    constexpr const bool kHasReturnValue = !std::is_same_v<void, std::invoke_result_t<Func>>;

    // https://stackoverflow.com/questions/78325310/boost-asio-what-executor-is-associated-with-the-default-completion-tokens-shou
    auto original_executor = co_await boost::asio::this_coro::executor;
    auto to_thread_pool = boost::asio::bind_executor(thread_pool_, boost::asio::use_awaitable);
    co_await boost::asio::post(to_thread_pool);
    if constexpr (kHasReturnValue) {
      auto result = func();
      co_await boost::asio::post(original_executor, boost::asio::use_awaitable);
      co_return result;
    } else {
      co_await boost::asio::post(original_executor, boost::asio::use_awaitable);
    }
  }

private:
  boost::asio::thread_pool thread_pool_;
};

}
