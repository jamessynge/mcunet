#include "connection.h"

// TODO(jamessynge): Trim down the includes after writing tests.
#include "extras/test_tools/string_io_stream_impl.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mcucore/extras/test_tools/print_to_std_string.h"
#include "mcucore/extras/test_tools/print_value_to_std_string.h"
#include "mcucore/extras/test_tools/sample_printable.h"

namespace mcunet {
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

// constexpr uint8_t kWriteBufferSize = 8;

// TEST(ConnectionTest, NoFixtureTest) {
//   uint8_t write_buffer[kWriteBufferSize];
//   FakeWriteBufferedWrappedClientConnection conn(write_buffer,
//   kWriteBufferSize);
// }

// TEST(NoFixture_ConnectionTest, NoFixtureTest) {
//   // TODO(jamessynge): Describe if not really obvious.
//   EXPECT_EQ(1, 1);
// }

// class ConnectionTest : public testing::Test {
//  protected:
//   ConnectionTest () {}
//   void SetUp() override {}
// };

// TEST_F(ConnectionTest, FixturedTest) {
//   // TODO(jamessynge): Describe if not really obvious.
//   EXPECT_EQ(1, 1);
// }

}  // namespace
}  // namespace test
}  // namespace mcunet
