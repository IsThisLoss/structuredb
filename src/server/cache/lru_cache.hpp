#pragma once

#include <cstddef>
#include <list>
#include <unordered_map>
#include <utility>

namespace structuredb::server::cache {

/// @brief fixed-capacity Least-Recently-Used cache
///
/// Keeps at most @c capacity entries. Reading (Get) or writing (Put) an entry
/// marks it most-recently-used; once the cache is full, inserting a new key
/// evicts the least-recently-used entry.
///
/// Lookup, insertion and eviction are all O(1): a doubly linked list keeps the
/// usage order (most-recently-used at the front) while a hash map gives direct
/// access to each list node.
///
/// @note Not thread-safe. Intended for the single-threaded cooperative
///       coroutine model used across the server: the io_context runs one
///       coroutine at a time, so the maps are only touched between co_await
///       points and no std::mutex is needed. Do not hold a returned pointer
///       across a co_await that may mutate the same cache.
template <typename Key, typename Value>
class LruCache {
public:
  explicit LruCache(size_t capacity) : capacity_{capacity} {}

  /// @brief looks up @p key, marking it most-recently-used on hit
  /// @returns pointer to the stored value, or nullptr if the key is absent
  ///
  /// The returned pointer stays valid until the next mutating call on the
  /// cache (a Put, or another Get is fine — only Put can evict).
  Value* Get(const Key& key) {
    auto it = index_.find(key);
    if (it == index_.end()) {
      return nullptr;
    }
    // move the touched node to the front (most-recently-used)
    entries_.splice(entries_.begin(), entries_, it->second);
    return &it->second->second;
  }

  /// @brief inserts or updates @p key, marking it most-recently-used
  ///
  /// Evicts the least-recently-used entry if the cache exceeds its capacity.
  void Put(const Key& key, Value value) {
    if (capacity_ == 0) {
      return;
    }

    auto it = index_.find(key);
    if (it != index_.end()) {
      it->second->second = std::move(value);
      entries_.splice(entries_.begin(), entries_, it->second);
      return;
    }

    entries_.emplace_front(key, std::move(value));
    index_[key] = entries_.begin();

    if (index_.size() > capacity_) {
      // evict least-recently-used (the back of the list)
      index_.erase(entries_.back().first);
      entries_.pop_back();
    }
  }

  bool Contains(const Key& key) const {
    return index_.contains(key);
  }

  size_t Size() const {
    return index_.size();
  }

  size_t Capacity() const {
    return capacity_;
  }

  void Clear() {
    entries_.clear();
    index_.clear();
  }

private:
  using Entry = std::pair<Key, Value>;

  // most-recently-used at the front, least-recently-used at the back
  std::list<Entry> entries_;
  std::unordered_map<Key, typename std::list<Entry>::iterator> index_;
  size_t capacity_;
};

}
