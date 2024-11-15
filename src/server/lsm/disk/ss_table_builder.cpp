#include "ss_table_builder.hpp"

namespace structuredb::server::lsm::disk {

Awaitable<SSTableBuilder> SSTableBuilder::Create(io::FileWriter::Ptr file_writer, const int64_t page_size) {
  SSTableBuilder builder{std::move(file_writer), page_size};
  co_await builder.Init();
  co_return builder;
}

SSTableBuilder::SSTableBuilder(io::FileWriter::Ptr file_writer, const int64_t page_size)
  : header_{
    .page_size = page_size,
    .page_count = 0,
  }
  , sdb_writer_{std::move(file_writer)}
  , page_builder_{page_size}
{
}

Awaitable<void> SSTableBuilder::Init() {
  // reserve space for header
  co_await SSTableHeader::Flush(sdb_writer_, header_);
  is_initialized_ = true;
}

Awaitable<void> SSTableBuilder::Add(const std::string& key, const std::string& value) {
  assert(is_initialized_);

  if (!page_builder_.IsEnoughPlace(key, value)) {
    co_await FlushPage();
  }

  page_builder_.Add(key, value);
}

Awaitable<void> SSTableBuilder::Finish() && {
  if (!page_builder_.IsEmpty()) {
    co_await FlushPage();
  }

  co_await sdb_writer_.Rewind();
  co_await SSTableHeader::Flush(sdb_writer_, header_);
}

Awaitable<void> SSTableBuilder::FlushPage() {
    co_await page_builder_.Flush(sdb_writer_);
    page_builder_.Clear();
    header_.page_count++;
}

}
