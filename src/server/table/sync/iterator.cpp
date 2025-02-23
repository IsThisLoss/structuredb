#include "iterator.hpp"

namespace structuredb::server::table::sync {

Iterator::Iterator(io::Manager& io_manager, table::Iterator::Ptr impl)
  : io_manager_{io_manager}
  , impl_{std::move(impl)}
{
  assert(impl_);
}

bool Iterator::HasMore() {
  return impl_->HasMore();
}

Row Iterator::Next() {
  return io_manager_.RunSync(impl_->Next());
}

}
