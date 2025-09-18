#pragma once

#include <io/shared_mutex.hpp>

namespace structuredb::server::transaction {

/// @brief Manages shared and exclusive locks for database objects
class LockManager {
public:
    using Ptr = std::shared_ptr<LockManager>;

    explicit LockManager(boost::asio::io_context& io_context);
  
    LockManager(const LockManager&) = delete;

    ~LockManager() = default;

    /// @brief Acquire a shared lock for the given key.
    Awaitable<void> LockShared(const std::string& key);

    /// @brief Release a shared lock for the given key.
    Awaitable<void> UnlockShared(const std::string& key);

    /// @brief Acquire an exclusive lock for the given key.
    Awaitable<void> LockExclusive(const std::string& key);

    /// @brief Release an exclusive lock for the given key.
    Awaitable<void> UnlockExclusive(const std::string& key);
private:
    using SharedMutexPtr = std::unique_ptr<io::SharedMutex>;

    struct Lock {
      size_t count{0};
      SharedMutexPtr mutex{nullptr};
    };

    boost::asio::io_context& io_context_;
    io::SharedMutex mutex_;
    std::unordered_map<std::string, Lock> locks_;

    Awaitable<void> AcquireLock(const std::string& key, bool exclusive);
    Awaitable<void> ReleaseLock(const std::string& key, bool exclusive);
};

} // namespace structuredb::transaction
