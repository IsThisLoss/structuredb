#pragma once

#include <boost/asio/awaitable.hpp>

namespace structuredb::server {

template <typename T>
using Awaitable = boost::asio::awaitable<T>;

}
