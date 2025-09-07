#include <iostream>
#include <fstream>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>

#include <lsm/disk/page_header.hpp>
#include <lsm/disk/ss_table_header.hpp>
#include <lsm/types.hpp>
#include <sdb/buffer_reader.hpp>

ABSL_FLAG(std::string, file_path, "data.sst.sdb", "Path to the StructureDB file");

namespace server = structuredb::server;

namespace {

server::lsm::disk::SSTableHeader ReadTableHeader(std::ifstream& file) {
  const auto table_header_size = server::lsm::disk::SSTableHeader::SdbSize();
  std::vector<char> buffer(table_header_size);
  file.read(buffer.data(), static_cast<ssize_t>(buffer.size()));

  server::sdb::BufferReader reader{std::move(buffer)};
  server::lsm::disk::SSTableHeader table_header{};
  server::lsm::disk::Read(reader, table_header);
  return table_header;
}

}

int main(int argc, char** argv) {
  const auto args = absl::ParseCommandLine(argc, argv);
  const std::string file_path = absl::GetFlag(FLAGS_file_path);

  if (!file_path.ends_with(".sst.sdb")) {
    std::cerr << "For now only .sst.sdb files are allowed" << std::endl;
    return 1;
  }

  std::cout << "Going to read file: " << file_path << std::endl;
  std::ifstream file(file_path, std::ios::binary);
  if (!file) {
    std::cerr << "Failed to open file: " << file_path << std::endl;
    return 1;
  }

  const auto table_header = ReadTableHeader(file);
  std::cout << "Table header: page_size=" << table_header.page_size
            << ", page_count=" << table_header.page_count
            << ", max_seq_no=" << table_header.max_seq_no << std::endl;

  for (int64_t i = 0; !file.eof() && i < table_header.page_count; ++i) {
    std::vector<char> page_buffer(table_header.page_size);
    file.read(page_buffer.data(), static_cast<ssize_t>(page_buffer.size()));
    server::sdb::BufferReader page_reader{std::move(page_buffer)};

    server::lsm::disk::PageHeader page_header{};
    server::lsm::disk::Read(page_reader, page_header);

    std::cout << "  Page " << i << ": count=" << page_header.count
              << ", checksum=" << page_header.checksum << std::endl;

    for (int64_t j = 0; j < page_header.count; ++j) {
      server::lsm::Record record{};
      server::lsm::Read(page_reader, record);

      std::cout << "    Record " << j << ": key=\"" << record.key
                << "\", seq_no=" << record.seq_no
                << ", value=\"" << record.value << "\"" << std::endl;
    }
  }

  return 0;
}
