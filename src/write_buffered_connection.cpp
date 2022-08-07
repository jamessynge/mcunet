#include "write_buffered_connection.h"

#include <McuCore.h>

namespace mcunet {

WriteBufferedConnection::WriteBufferedConnection(uint8_t *write_buffer,
                                                 uint8_t write_buffer_limit,
                                                 Client &client)
    : write_buffer_(write_buffer),
      write_buffer_limit_(write_buffer_limit),
      write_buffer_size_(0),
      client_(client) {
  MCU_DCHECK(write_buffer != nullptr);
  MCU_DCHECK(write_buffer_limit > 0);
}

WriteBufferedConnection::~WriteBufferedConnection() {
  MCU_VLOG(9) << MCU_PSD("WriteBufferedConnection@") << this
              << MCU_PSD(" dtor");
  flush();
}

size_t WriteBufferedConnection::write(uint8_t b) {
  MCU_VLOG(9) << MCU_PSD("WriteBufferedConnection@") << this
              << MCU_PSD("::write b=") << mcucore::BaseHex << (b + 0);
  MCU_DCHECK_LE(write_buffer_size_, write_buffer_limit_);
  bool ok_to_write;
  if (write_buffer_size_ >= write_buffer_limit_) {
    ok_to_write = FlushInternal();
  } else {
    ok_to_write = getWriteError() == 0;
  }
  if (ok_to_write) {
    write_buffer_[write_buffer_size_++] = b;
    return 1;
  } else {
    return 0;
  }
}

size_t WriteBufferedConnection::write(const uint8_t *buf, const size_t size) {
  MCU_VLOG(9) << MCU_PSD("WriteBufferedConnection@") << this
              << MCU_PSD("::write sizeb=") << size;
  MCU_DCHECK_LE(write_buffer_size_, write_buffer_limit_);
  // buf should not overlap with write_buffer_.
  MCU_DCHECK((buf + size <= write_buffer_) ^
             (write_buffer_ + write_buffer_limit_ <= buf))
      << mcucore::BaseHex << buf << ' ' << size << ' ' << write_buffer_ << ' '
      << write_buffer_limit_;

  if (getWriteError() != 0) {
    return 0;
  }

  // I tried adding an optimization here for the case where we'll have to do at
  // least two flush calls here (i.e. if the amount of data in buf, plus the
  // already buffered data, is at least twice the size of the write buffer).
  // However, it complicates this code enough that it doesn't seem worth it,
  // especially given that I don't expect the amount of data in buf to exceed
  // the size of the write_buffer_ very often in the embedded setting.

  size_t remaining = size;
  while (remaining > 0) {
    MCU_DCHECK_LE(write_buffer_size_, write_buffer_limit_);
    size_t room = write_buffer_limit_ - write_buffer_size_;
    if (room == 0) {
      if (!FlushInternal()) {
        MCU_VLOG(9) << MCU_PSD("WriteBufferedConnection@") << this
                    << MCU_PSD("::write: !ok after flush");
        return 0;
      }
    }
    if (room > remaining) {
      room = remaining;
    }
    memcpy(write_buffer_ + write_buffer_size_, buf, room);
    write_buffer_size_ += room;
    buf += room;
    remaining -= room;
  }
  return size;
}

int WriteBufferedConnection::availableForWrite() {
  if (getWriteError() == 0) {
    return write_buffer_limit_ - write_buffer_size_;
  }
  return -1;
}

int WriteBufferedConnection::available() { return client_.available(); }

int WriteBufferedConnection::read() { return client_.read(); }

int WriteBufferedConnection::read(uint8_t *buf, size_t size) {
  return client_.read(buf, size);
}

int WriteBufferedConnection::peek() { return client_.peek(); }

uint8_t WriteBufferedConnection::connected() { return client_.connected(); }

void WriteBufferedConnection::flush() {
  if (write_buffer_size_ == 0) {
    return;
  }
  FlushInternal();
}

bool WriteBufferedConnection::FlushInternal() {
  MCU_DCHECK_LT(0, write_buffer_size_);
  MCU_DCHECK_LE(write_buffer_size_, write_buffer_limit_);

  if (getWriteError() != 0) {
    return false;
  }

  // Ethernet5500's send() function is sort of non-blocking. It will write at
  // most 2KB of data at a time, but will wait for that room to be available.
  // Therefore we use the assumption that client_.write() can always write
  // fewer bytes than we passed in, we use the loop below as many times as
  // necessary.

  uint8_t *buf = write_buffer_;
  size_t size = write_buffer_size_;
  size_t result = 0;
  do {
    auto wrote = client_.write(buf, size);
    MCU_VLOG(9) << MCU_PSD("write ") << size << MCU_PSD(", wrote ") << wrote;
    MCU_DCHECK_GE(size, wrote);
    // We assume here that writing any positive number of bytes implies that
    // there was no underlying write error, so we don't ask the client whether
    // there was an error. Further, we expect that writing all of the data is
    // the most likely outcome, so we check for that first.
    if (wrote == size) {
      write_buffer_size_ = 0;
      return true;
    } else if (wrote > 0) {
      buf += wrote;
      size -= wrote;
      result += wrote;
    } else {
      MCU_DCHECK_EQ(wrote, 0);
      const auto client_error = client_.getWriteError();
      if (client_error != 0) {
        setWriteError(client_error);
        // We assume here that the connection is going to be closed, so we don't
        // bother with "removing" any successfully written bytes from the write
        // buffer.
      } else {
        // We made no progress, yet there was no error. That seems wrong.
        setWriteError(kBlockedFlush);
        MCU_VLOG(3) << MCU_PSD("no error, wrote 0");
      }
      return false;
    }
    MCU_DCHECK_GT(size, 0);
  } while (true);
}

}  // namespace mcunet
