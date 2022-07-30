#include "extras/test_tools/string_utils.h"

#include <McuCore.h>

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/strings/escaping.h"

namespace mcunet {
namespace test {

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
    const std::string& full_request) {
  DLOG(INFO) << "GenerateMultipleRequestPartitions; full_request (size="
             << full_request.size() << "):\n"
             << full_request;
  std::vector<std::vector<std::string>> partitions;
  size_t n = std::min(static_cast<size_t>(mcucore::StringView::kMaxSize),
                      full_request.size());
  bool first = true;
  do {
    auto partition = SplitEveryN(full_request, n);
    partitions.push_back(partition);
    if (first) {
      // Start with an empty string.
      partition.insert(partition.begin(), "");
      first = false;
    }
  } while (--n > 0);
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

}  // namespace test
}  // namespace mcunet
