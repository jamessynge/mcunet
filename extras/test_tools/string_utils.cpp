#include "extras/test_tools/string_utils.h"

#include <McuCore.h>

#include <algorithm>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/strings/escaping.h"

namespace mcunet {
namespace test {

std::vector<std::string> AllRegisteredMethodNames() {
  return {
      "ACL",
      "BASELINE-CONTROL",
      "BIND",
      "CHECKIN",
      "CHECKOUT",
      "CONNECT",
      "COPY",
      "DELETE",
      "GET",
      "HEAD",
      "LABEL",
      "LINK",
      "LOCK",
      "MERGE",
      "MKACTIVITY",
      "MKCALENDAR",
      "MKCOL",
      "MKREDIRECTREF",
      "MKWORKSPACE",
      "MOVE",
      "OPTIONS",
      "ORDERPATCH",
      "PATCH",
      "POST",
      "PRI",
      "PROPFIND",
      "PROPPATCH",
      "PUT",
      "REBIND",
      "REPORT",
      "SEARCH",
      "TRACE",
      "UNBIND",
      "UNCHECKOUT",
      "UNLINK",
      "UNLOCK",
      "UPDATE",
      "UPDATEREDIRECTREF",
      "VERSION-CONTROL",
  };
}

std::vector<std::string> SplitEveryN(const std::string& full_request,
                                     const size_t n) {
  DVLOG(5) << "SplitEveryN " << n
           << ", full_request.size: " << full_request.size();
  CHECK_LE(n, mcucore::StringView::kMaxSize);
  std::vector<std::string> partition;
  for (size_t pos = 0; pos < full_request.size(); pos += n) {
    DVLOG(6) << "pos: " << pos << ", pos+n: " << pos + n;
    partition.push_back(full_request.substr(pos, n));
    DVLOG(6) << "part: \"" << absl::CHexEscape(partition.back()) << "\"";
  }
  return partition;
}

std::vector<std::vector<std::string>> GenerateMultipleRequestPartitions(
    const std::string& full_request, const size_t max_decode_buffer_size,
    size_t max_literal_match_size) {
  QCHECK_GE(max_decode_buffer_size, max_literal_match_size);
  QCHECK_GE(max_literal_match_size, 1);
  DLOG(INFO) << "GenerateMultipleRequestPartitions; full_request (size="
             << full_request.size() << "):\n"
             << full_request;
  std::vector<std::vector<std::string>> partitions;
  partitions.push_back({full_request});
  partitions.push_back(SplitEveryN(full_request, max_decode_buffer_size));
  if (max_literal_match_size == max_decode_buffer_size) {
    max_literal_match_size -= 1;
  }
  for (size_t buffer_size = 1; buffer_size <= max_literal_match_size;
       ++buffer_size) {
    auto partition = SplitEveryN(full_request, buffer_size);
    partitions.push_back(partition);
  }
  return partitions;
}

std::string AppendRemainder(const std::string& buffer,
                            const std::vector<std::string>& partition,
                            int ndx) {
  std::string result = buffer;
  for (; ndx < partition.size(); ++ndx) {
    result += partition[ndx];
  }
  return result;
}

std::string AllCharsExcept(bool (*excluding)(char c)) {
  std::string result;
  char c = std::numeric_limits<char>::min();
  do {
    if (!excluding(c)) {
      result.push_back(c);
    }
  } while (c++ < std::numeric_limits<char>::max());
  return result;
}

std::string AllCharsExcept(int (*excluding)(int c)) {
  std::string result;
  char c = std::numeric_limits<char>::min();
  do {
    if (!excluding(c)) {
      result.push_back(c);
    }
  } while (c++ < std::numeric_limits<char>::max());
  return result;
}

std::string EveryChar() {
  std::string result;
  result.reserve(256);
  char c = std::numeric_limits<char>::min();
  do {
    result.push_back(c);
  } while (c++ < std::numeric_limits<char>::max());
  return result;
}

std::string PercentEncodeChar(const char c) {
  const auto i = static_cast<uint_fast8_t>(c);
  std::string result("%--");
  result[1] = mcucore::NibbleToAsciiHex((i & 0xF0) >> 4);
  result[2] = mcucore::NibbleToAsciiHex(i & 0x0F);
  return result;
}

std::string PercentEncodeAllChars(std::string_view input) {
  std::string result;
  result.reserve(input.size() * 3);
  for (const char c : input) {
    result += PercentEncodeChar(c);
  }
  return result;
}

std::vector<std::string> GenerateInvalidPercentEncodedChar() {
  std::vector<std::string> result;
  for (const char c :
       AllCharsExcept([](char c) -> bool { return isxdigit(c); })) {
    result.push_back(std::string({'%', '0', c}));
    result.push_back(std::string({'%', c, '0'}));
  }
  return result;
}

}  // namespace test
}  // namespace mcunet
