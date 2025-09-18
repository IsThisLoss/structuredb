#include "lock_manager.hpp"

namespace structuredb::server::transaction {

LockManager::LockManager(boost::asio::io_context& io_context)
  : io_context_{io_context}, mutex_{io_context}
{}

Awaitable<void> LockManager::AcquireLock(const std::string& key, bool exclusive) {
  co_await mutex_.LockExclusive();
  auto [it, is_new] = locks_.try_emplace(key, Lock{});
  auto& lock = it->second;
  if (is_new) {
    lock.mutex = std::make_unique<io::SharedMutex>(io_context_);
  }
  lock.count++;
  co_await mutex_.UnlockExclusive();

  if (exclusive) {
    co_await lock.mutex->LockExclusive();
  } else {
    co_await lock.mutex->LockShared();
  }
}

Awaitable<void> LockManager::ReleaseLock(const std::string& key, bool exclusive) {
  co_await mutex_.LockExclusive();
  auto it = locks_.find(key);
  if (it == locks_.end()) {
    co_await mutex_.UnlockExclusive();
    co_return;
  }
  auto& lock = it->second;

  if (exclusive) {
    co_await lock.mutex->UnlockExclusive();
  } else {
    co_await lock.mutex->UnlockShared();
  }

  lock.count--;
  if (lock.count == 0) {
    locks_.erase(it);
  }
  co_await mutex_.UnlockExclusive();
}

Awaitable<void> LockManager::LockShared(const std::string& key) {
  co_await AcquireLock(key, false);
}

Awaitable<void> LockManager::UnlockShared(const std::string& key) {
  co_await ReleaseLock(key, false);
}

Awaitable<void> LockManager::LockExclusive(const std::string& key) {
  co_await AcquireLock(key, true);
}

Awaitable<void> LockManager::UnlockExclusive(const std::string& key) {
  co_await ReleaseLock(key, true);
}

}
