#include "ss_table_builder.hpp"

#include <algorithm>
#include <sdb/buffer_writer.hpp>

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
    .max_seq_no = 0,
  },
  page_builder_{page_size},
  file_writer_{std::move(file_writer)}
{
}

Awaitable<void> SSTableBuilder::Init() {
  // reserve space for header
  co_await FlushHeader();
  is_initialized_ = true;
}

Awaitable<void> SSTableBuilder::Add(const Record& record) {
  assert(is_initialized_);

  sdb::BufferWriter writer;
  Write(writer, record);
  const auto raw = std::move(writer).Extract();
  if (!page_builder_.IsEnoughSpace(raw.size())) {
    co_await FlushPage();
  }
  page_builder_.AddRecord(raw);
  header_.max_seq_no = std::max(header_.max_seq_no, record.seq_no);
}

Awaitable<void> SSTableBuilder::Finish() && {
  if (!page_builder_.Empty()) {
    co_await FlushPage();
  }

  co_await file_writer_->Rewind();
  co_await FlushHeader();
}

Awaitable<void> SSTableBuilder::FlushHeader() {
  sdb::BufferWriter writer;
  Write(writer, header_);
  const auto raw = std::move(writer).Extract();
  co_await file_writer_->Write(raw.data(), raw.size());
}

Awaitable<void> SSTableBuilder::FlushPage() {
  assert(!page_builder_.Empty());

  auto raw = std::move(page_builder_).Extract();
  co_await file_writer_->Write(raw.data(), raw.size());

  page_builder_.Clear();
  header_.page_count++;
}

}
