#include "extras/host/ethernet5500/host_socket_info.h"

// TODO(jamessynge): Write tests of HostSocketInfo.

// TODO(jamessynge): Trim down the includes after writing tests.
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mcucore/extras/test_tools/print_to_std_string.h"
#include "mcucore/extras/test_tools/print_value_to_std_string.h"
#include "mcucore/extras/test_tools/sample_printable.h"

namespace mcunet_host {
namespace test {
namespace {

// TODO(jamessynge): Trim down the using declarations after writing tests.
using ::mcucore::PrintValueToStdString;
using ::mcucore::test::PrintToStdString;
using ::mcucore::test::SamplePrintable;
using ::testing::AllOf;
using ::testing::AnyNumber;
using ::testing::AnyOf;
using ::testing::Contains;
using ::testing::ContainsRegex;
using ::testing::ElementsAre;
using ::testing::EndsWith;
using ::testing::Eq;
using ::testing::HasSubstr;
using ::testing::InSequence;
using ::testing::IsEmpty;
using ::testing::Mock;
using ::testing::NiceMock;
using ::testing::Not;
using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SizeIs;
using ::testing::StartsWith;
using ::testing::StrictMock;

TEST(NoFixture_HostSocketInfoTest, NoFixtureTest) {
  // TODO(jamessynge): Describe if not really obvious.
  EXPECT_EQ(1, 1);
}

class HostSocketInfoTest : public testing::Test {
 protected:
  HostSocketInfoTest() {}
  void SetUp() override {}
};

TEST_F(HostSocketInfoTest, FixturedTest) {
  // TODO(jamessynge): Describe if not really obvious.
  EXPECT_EQ(1, 1);
}

}  // namespace
}  // namespace test
}  // namespace mcunet_host
