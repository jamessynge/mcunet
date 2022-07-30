#ifndef MCUNET_EXTRAS_TEST_TOOLS_STRING_UTILS_H_
#define MCUNET_EXTRAS_TEST_TOOLS_STRING_UTILS_H_

// Helper functions focused on testing the HTTP request decoder.

#include <string>
#include <vector>

namespace mcunet {
namespace test {

std::vector<std::string> SplitEveryN(const std::string& full_request,
                                     const size_t n);

std::vector<std::vector<std::string>> GenerateMultipleRequestPartitions(
    const std::string& full_request);

std::string AppendRemainder(const std::string& buffer,
                            const std::vector<std::string>& partition, int ndx);

// Returns all char values except those where function `excluding` returns true.
std::string AllCharsExcept(bool (*excluding)(char c));
std::string AllCharsExcept(int (*excluding)(int c));

std::vector<std::string> AllRegisteredMethodNames();

}  // namespace test
}  // namespace mcunet

#endif  // MCUNET_EXTRAS_TEST_TOOLS_STRING_UTILS_H_
