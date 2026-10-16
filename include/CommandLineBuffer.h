#pragma once

#include <stddef.h>

// Bounded newline framing. A damaged/oversized line is discarded in full.
template <size_t Capacity>
class CommandLineBuffer {
 public:
  void reset(bool discardUntilNewline = false) {
    length_ = 0;
    dropping_ = discardUntilNewline;
  }

  template <typename Submit>
  void push(char c, Submit submit) {
    if (c == '\n') {
      if (!dropping_ && length_) {
        data_[length_] = '\0';
        submit(data_);
      }
      reset();
    } else if (c == '\0') {
      reset(true);
    } else if (c != '\r' && !dropping_) {
      if (length_ < Capacity - 1) {
        data_[length_++] = c;
      } else {
        reset(true);
      }
    }
  }

 private:
  char data_[Capacity] = {};
  size_t length_ = 0;
  bool dropping_ = false;
};
