#include "write_buffered_connection.h"

#include <array>
#include <string>
#include <vector>

#include "extras/test_tools/fake_write_buffered_connection.h"
#include "extras/test_tools/mock_client.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace mcunet {
namespace test {
namespace {

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::Invoke;
using ::testing::IsEmpty;
using ::testing::Return;

constexpr size_t kWriteBufferSize = 16;

class WriteBufferedConnectionTest : public testing::Test {
 protected:
  void SetUp() override {
    ON_CALL(mock_client_, write(_, _))
        .WillByDefault(Invoke([this](const uint8_t* data, size_t size) {
          flushed_data_.insert(flushed_data_.end(), data, data + size);
          return size;
        }));

    // WriteBufferedConnection never uses the client's write(uint8_t) method.
    EXPECT_CALL(mock_client_, write(_)).Times(0);
  }

  std::vector<uint8_t> flushed_data_;
  MockClient mock_client_;
  std::array<uint8_t, kWriteBufferSize> write_buffer_;
};

TEST_F(WriteBufferedConnectionTest, WriteByteWithFlush) {
  // Write bytes one at a time, eventually causing a flush.
  FakeWriteBufferedConnection conn{mock_client_, 2, write_buffer_};

  // Won't write anything during this loop.
  EXPECT_CALL(mock_client_, write(_, _)).Times(0);
  for (int ndx = 0; ndx < kWriteBufferSize; ++ndx) {
    EXPECT_EQ(conn.availableForWrite(), kWriteBufferSize - ndx);
    EXPECT_EQ(conn.getWriteError(), 0);
    char c = 'a' + ndx;
    EXPECT_EQ(conn.print(c), 1);
  }

  EXPECT_EQ(conn.getWriteError(), 0);

  // Haven't written anything yet, though the buffer should be full.
  EXPECT_EQ(conn.availableForWrite(), 0);
  EXPECT_THAT(flushed_data_, IsEmpty());

  // Write another byte, which should trigger a flush.
  EXPECT_CALL(mock_client_, write(_, _)).Times(1);
  EXPECT_EQ(conn.write(1), 1);

  EXPECT_THAT(flushed_data_,
              ElementsAre('a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k',
                          'l', 'm', 'n', 'o', 'p'));

  // We can expect a write of the buffered data when the conn
  // instance is deleted (i.e. due to the flush call).
  EXPECT_CALL(mock_client_, write(write_buffer_.data(), 1));
}

TEST_F(WriteBufferedConnectionTest, WriteBufferWithFinalFlush) {
  FakeWriteBufferedConnection conn{mock_client_, 2, write_buffer_};
  EXPECT_EQ(conn.availableForWrite(), kWriteBufferSize);

  // If we don't write too much, then write won't flush the buffer.
  std::string data(kWriteBufferSize, 'a');
  LOG(INFO) << "data: " << data << ", size=" << data.size();
  EXPECT_CALL(mock_client_, write(_)).Times(0);
  EXPECT_CALL(mock_client_, write(_, _)).Times(0);
  EXPECT_EQ(conn.print(data.c_str()), data.size());
  EXPECT_EQ(conn.availableForWrite(), 0);
  EXPECT_EQ(conn.getWriteError(), 0);

  // We can expect a write of the buffered data when the conn
  // instance is deleted (i.e. due to the flush call).
  EXPECT_CALL(mock_client_, write(write_buffer_.data(), data.size()));
}

TEST_F(WriteBufferedConnectionTest, WriteTriggersFlush) {
  FakeWriteBufferedConnection conn{mock_client_, 2, write_buffer_};

  // If we write more than the available space, it will trigger a flush of the
  // write buffer.
  std::vector<uint8_t> data(kWriteBufferSize + 1, 7);
  EXPECT_CALL(mock_client_, write(write_buffer_.data(), kWriteBufferSize))
      .Times(1);
  EXPECT_EQ(conn.write(data.data(), data.size()), data.size());
  EXPECT_EQ(conn.getWriteError(), 0);

  // There was one more byte than could fit in the buffer, so that one is the
  // only one left after the first kWriteBufferSize bytes were flushed.
  EXPECT_EQ(conn.availableForWrite(), kWriteBufferSize - 1);

  // If we flush now, the one byte currently buffered will be written.
  EXPECT_CALL(mock_client_, write(write_buffer_.data(), 1)).Times(1);
  conn.flush();

  EXPECT_CALL(mock_client_, write(_, _)).Times(0);
}

TEST_F(WriteBufferedConnectionTest, WriteTriggersFlushWithError) {
  FakeWriteBufferedConnection conn{mock_client_, 2, write_buffer_};

  // If we write more than the available space, it will trigger a flush of the
  // write buffer. If that returns 0, then we've failed to write any bytes.
  std::vector<uint8_t> data(kWriteBufferSize + 1, 7);
  EXPECT_CALL(mock_client_, write(write_buffer_.data(), kWriteBufferSize))
      .WillRepeatedly(Return(0));
  EXPECT_EQ(conn.write(data.data(), data.size()), 0);
  EXPECT_EQ(conn.getWriteError(), WriteBufferedConnection::kBlockedFlush);
  EXPECT_EQ(conn.availableForWrite(), -1);
}

TEST_F(WriteBufferedConnectionTest, WriteWithError) {
  FakeWriteBufferedConnection conn{mock_client_, 2, write_buffer_};
  EXPECT_EQ(conn.availableForWrite(), kWriteBufferSize);
  conn.setWriteError(1);
  EXPECT_EQ(conn.availableForWrite(), -1);
  uint8_t b = 7;
  EXPECT_EQ(conn.write(b), 0);
  EXPECT_EQ(conn.write(&b, sizeof b), 0);
}

TEST_F(WriteBufferedConnectionTest, WriteWithPartialFlush) {
  // Write bytes one at a time, eventually causing a flush, which will only
  // partially write the bytes on the first try.
  FakeWriteBufferedConnection conn{mock_client_, 2, write_buffer_};

  EXPECT_CALL(mock_client_, write(_, _))
      .WillRepeatedly(Invoke([this](const uint8_t* data, size_t size) {
        if (size > 2) {
          size /= 2;
        }
        flushed_data_.insert(flushed_data_.end(), data, data + size);
        return size;
      }));

  EXPECT_EQ(conn.print("abcdefghijklmnopqrstuvwxyz"), 26);
  conn.flush();
  EXPECT_THAT(flushed_data_,
              ElementsAre('a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k',
                          'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                          'w', 'x', 'y', 'z'));
}

TEST_F(WriteBufferedConnectionTest, FlushWithError) {
  FakeWriteBufferedConnection conn{mock_client_, 2, write_buffer_};
  conn.write(123);
  EXPECT_CALL(mock_client_, write(_, _))
      .WillOnce(Invoke([this](const uint8_t* data, size_t size) {
        mock_client_.setWriteError(321);
        return 0;
      }));
  conn.flush();
  EXPECT_EQ(conn.getWriteError(), 321);
}

TEST_F(WriteBufferedConnectionTest, ReadForwardedToClient) {
  FakeWriteBufferedConnection conn{mock_client_, 2, write_buffer_};

  EXPECT_CALL(mock_client_, read()).WillOnce(Return('a'));
  EXPECT_EQ(conn.read(), 'a');

  EXPECT_CALL(mock_client_, read()).WillOnce(Return(1000));
  EXPECT_EQ(conn.read(), 1000);

  EXPECT_CALL(mock_client_, read()).WillOnce(Return(-1));
  EXPECT_EQ(conn.read(), -1);

  uint8_t b;
  EXPECT_CALL(mock_client_, read(&b, 1)).WillOnce(Return(-1));
  EXPECT_EQ(conn.read(&b, sizeof b), -1);

  EXPECT_CALL(mock_client_, read(&b, 1)).WillOnce(Return(0));
  EXPECT_EQ(conn.read(&b, sizeof b), 0);

  EXPECT_CALL(mock_client_, read(&b, 1)).WillOnce(Return(1));
  EXPECT_EQ(conn.read(&b, sizeof b), 1);
}

TEST_F(WriteBufferedConnectionTest, MiscForwardedToClient) {
  FakeWriteBufferedConnection conn{mock_client_, 7, write_buffer_};
  EXPECT_EQ(conn.sock_num(), 7);

  EXPECT_CALL(mock_client_, available).WillOnce(Return(-1));
  EXPECT_EQ(conn.available(), -1);

  EXPECT_CALL(mock_client_, available).WillOnce(Return(3));
  EXPECT_EQ(conn.available(), 3);

  EXPECT_CALL(mock_client_, peek).WillOnce(Return(-1));
  EXPECT_EQ(conn.peek(), -1);

  EXPECT_CALL(mock_client_, peek).WillOnce(Return(0));
  EXPECT_EQ(conn.peek(), 0);
}

TEST_F(WriteBufferedConnectionTest, ConnectAndClose) {
  FakeWriteBufferedConnection conn{mock_client_, 2, write_buffer_};

  EXPECT_CALL(mock_client_, connected).WillOnce(Return(3));
  EXPECT_EQ(conn.connected(), 3);

  EXPECT_CALL(mock_client_, stop).Times(1);
  conn.close();

  EXPECT_CALL(mock_client_, connected).Times(0);
  EXPECT_EQ(conn.connected(), 0);
}

}  // namespace
}  // namespace test
}  // namespace mcunet
