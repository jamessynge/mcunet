#include "connection.h"

#include <McuCore.h>
#include <string.h>

namespace mcunet {

Connection::~Connection() {}

int Connection::read(uint8_t *buf, size_t size) {
  // This is a debug only check because our embedded systems are exceedingly
  // unlikely to have enough RAM to exceed this size.
  MCU_DCHECK_LE(size, mcucore::numeric_limits<int>::max());
  int result = 0;
  while (size > 0) {
    int c = read();
    if (c < 0) {
      break;
    }
    MCU_DCHECK_LT(c, 256) << MCU_FLASHSTR("c (") << c
                          << MCU_FLASHSTR(") should be in the range [0, 255]");
    *buf++ = c & 0xff;
    ++result;
    --size;
  }
  return result;
}

bool Connection::HasWriteError() { return getWriteError() != 0; }

bool Connection::IsOkToWrite() { return getWriteError() == 0 && connected(); }

WriteBufferedWrappedClientConnection::WriteBufferedWrappedClientConnection(
    uint8_t *write_buffer, uint8_t write_buffer_limit)
    : write_buffer_(write_buffer),
      write_buffer_limit_(write_buffer_limit),
      write_buffer_size_(0) {
  MCU_DCHECK(write_buffer != nullptr);
  MCU_DCHECK(write_buffer_limit > 0);
}

size_t WriteBufferedWrappedClientConnection::write(uint8_t b) {
  FlushIfFull();
  write_buffer_[write_buffer_size_++] = b;
  return 1;
}

size_t WriteBufferedWrappedClientConnection::write(const uint8_t *buf,
                                                   const size_t size) {
  // buf should not overlap with write_buffer_.
  MCU_DCHECK((buf + size <= write_buffer_) ^
             (write_buffer_ + write_buffer_limit_ <= buf))
      << mcucore::BaseHex << buf << ' ' << size << ' ' << write_buffer_ << ' '
      << write_buffer_limit_;

  if (write_buffer_size_ + size >= write_buffer_limit_ * 2) {
    // We'll have to do at least two flushes, maybe more, if we use the code
    // below, so this optimization limits us to two writes to the underlying
    // client (this assumes the underlying client doesn't break our writes up
    // into pieces).
    flush();
    if (IsOkToWrite()) {
      return WriteToClient(write_buffer_, write_buffer_size_);
    }
  }

  FlushIfFull();
  size_t remaining = size;
  while (remaining > 0) {
    size_t room = write_buffer_limit_ - write_buffer_size_;
    MCU_DCHECK_GT(room, 0);
    if (room > remaining) {
      room = remaining;
    }
    memcpy(write_buffer_ + write_buffer_size_, buf, room);
    write_buffer_size_ += room;
    buf += room;
    remaining -= room;
    FlushIfFull();
  }
  return size;
}

int WriteBufferedWrappedClientConnection::availableForWrite() {
  if (connected()) {
    return write_buffer_limit_ - write_buffer_size_;
  }
  return -1;
}

int WriteBufferedWrappedClientConnection::available() {
  if (connected()) {
    return client().available();
  }
  return -1;
}

int WriteBufferedWrappedClientConnection::read() { return client().read(); }

int WriteBufferedWrappedClientConnection::read(uint8_t *buf, size_t size) {
  return client().read(buf, size);
}

int WriteBufferedWrappedClientConnection::peek() { return client().peek(); }

void WriteBufferedWrappedClientConnection::flush() {
  if (write_buffer_size_ > 0) {
    MCU_VLOG(4) << MCU_FLASHSTR("write_buffer_size_=") << write_buffer_size_;
    auto wrote = WriteToClient(write_buffer_, write_buffer_size_);
    if (wrote == write_buffer_size_) {
      write_buffer_size_ = 0;
      return;
    }
    MCU_DCHECK(!IsOkToWrite())
        << getWriteError() << ',' << client().getWriteError() << ','
        << connected();
    MCU_DCHECK_EQ(wrote, 0) << wrote;
  }
}

uint8_t WriteBufferedWrappedClientConnection::connected() {
  return client().connected();
}

bool WriteBufferedWrappedClientConnection::HasWriteError() {
  if (getWriteError() == 0 && client().getWriteError() != 0) {
    setWriteError(client().getWriteError());
  }
  return getWriteError() != 0;
}

bool WriteBufferedWrappedClientConnection::IsOkToWrite() {
  if (!HasWriteError() && !connected()) {
    setWriteError(1);
  }
  return getWriteError() == 0;
}

void WriteBufferedWrappedClientConnection::FlushIfFull() {
  if (write_buffer_size_ >= write_buffer_limit_) {
    flush();
  }
}

size_t WriteBufferedWrappedClientConnection::WriteToClient(const uint8_t *buf,
                                                           size_t size) {
  MCU_DCHECK_NE(buf, nullptr);
  MCU_DCHECK_NE(size, 0);

  if (!IsOkToWrite()) {
    MCU_VLOG(5) << MCU_FLASHSTR("!IsOkToWrite");

    return 0;
  }
  size_t result = 0;
  while (size > 0) {
    auto wrote = client().write(buf, size);
    MCU_VLOG(4) << MCU_FLASHSTR("write ") << size << MCU_FLASHSTR(", wrote ")
                << wrote;
    if (wrote == size) {
      // This is the most likely situation on the very first pass through the
      // loop.
      return result + wrote;
    } else if (wrote > 0) {
      buf += wrote;
      size -= wrote;
      result += wrote;
    } else {
      // wrote == 0 implies an error, given that we're NOT using non-blocking
      // writes (i.e. that isn't a feature of the Arduino Print class). While
      // `result` bytes were successfully written, I'm assuming here that the
      // caller doesn't need to know that.
      setWriteError(1);
      return 0;
    }
  }
  return result;
}

}  // namespace mcunet
