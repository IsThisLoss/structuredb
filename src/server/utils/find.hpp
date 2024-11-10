#pragma once

#include <utility>

namespace structuredb::server::utils {

template <typename TMap, typename TKey>
std::decay_t<TMap>::mapped_type* FindOrNullptr(TMap& map, TKey&& key) {
  auto it = map.find(std::forward<TKey>(key));
  if (it != map.end()) {
    return &it->second;
  }
  return nullptr;
}

template <typename TMap, typename TKey>
const std::decay_t<TMap>::mapped_type* FindOrNullptr(const TMap& map, TKey&& key) {
  auto it = map.find(std::forward<TKey>(key));
  if (it != map.end()) {
    return &it->second;
  }
  return nullptr;
}

}
