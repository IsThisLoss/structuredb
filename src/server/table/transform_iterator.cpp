#include <memory>

#include "transform_iterator.hpp"

namespace structuredb::server::table {

TransformIterator::TransformIterator(Iterator::Ptr iter, TransformFunc transform)
    : iter_{std::move(iter)}, transform_{std::move(transform)} {}

bool TransformIterator::HasMore() {
  return iter_->HasMore();
}

Awaitable<Row> TransformIterator::Next() {
  auto row = co_await iter_->Next();
  co_return transform_(row);
}

Iterator::Ptr MakeTransformIterator(
    Iterator::Ptr iter,
    TransformIterator::TransformFunc transform
) {
  return std::make_shared<TransformIterator>(std::move(iter), std::move(transform));
}

} // namespace structuredb::server::table
