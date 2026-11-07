#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <sdb/buffer_writer.hpp>
#include <sdb/reader.hpp>
#include <wal/events/lsm_storage_upsert_event.hpp>

namespace structuredb::tests {

namespace {

class OrderedReader : public server::sdb::Reader {
public:
  void Read(char*, size_t) override { ADD_FAILURE() << "Unexpected raw read"; }
  std::string ReadString() override {
    calls.push_back("string");
    return "field";
  }
  int64_t ReadInt() override {
    calls.push_back("int");
    return 42;
  }
  bool ReadBool() override {
    ADD_FAILURE() << "Unexpected bool read";
    return false;
  }
  std::vector<std::string> calls;
};

}

TEST(LsmStorageUpsertEventTest, ParsesFieldsInWireOrder) {
  OrderedReader reader;
  auto event = server::wal::LsmStorageUpsertEvent::Parse(reader);
  EXPECT_EQ(reader.calls, (std::vector<std::string>{"string", "int", "string", "string"}));

  server::sdb::BufferWriter actual;
  event->Flush(actual);
  server::sdb::BufferWriter expected;
  expected.WriteString("field");
  expected.WriteInt(42);
  expected.WriteString("field");
  expected.WriteString("field");
  EXPECT_EQ(std::move(actual).Extract(), std::move(expected).Extract());
}

}
