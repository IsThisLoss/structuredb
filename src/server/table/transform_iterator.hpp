#pragma once

#include <functional>

#include "iterator.hpp"

namespace structuredb::server::table {

/// @brief applies a transformation function to each row returned by the underlying iterator.
class TransformIterator : public Iterator {
public:
  using TransformFunc = std::function<Row(const Row&)>;

  explicit TransformIterator(Iterator::Ptr iter, TransformFunc transform);

  bool HasMore() override;

  Awaitable<Row> Next() override;

private:
  Iterator::Ptr iter_;
  TransformFunc transform_;
};

Iterator::Ptr MakeTransformIterator(
    Iterator::Ptr iter,
    TransformIterator::TransformFunc transform
);

}
