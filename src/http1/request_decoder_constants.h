#ifndef MCUNET_SRC_HTTP1_REQUEST_DECODER_CONSTANTS_H_
#define MCUNET_SRC_HTTP1_REQUEST_DECODER_CONSTANTS_H_

// Enums for the HTTP/1.1 decoder. In a separate file to help with the use of
// make_enum_to_string.

#include <McuCore.h>
#include <stdint.h>  // pragma: keep standard include

#if MCU_HOST_TARGET
#include <ostream>  // pragma: keep standard include
#endif

namespace mcunet {
// I don't normally want to have many nested namespaces, but the decoder has a
// helper class and several enum definitions, with names that I'd like to keep
// short, and yet not readily conflict with other similar enums, so I've chosen
// to put them into a nested namespace.
namespace http1 {

// For decoding events that don't require more detail than just the type of
// event.
enum class EEvent : uint_fast8_t {
  // Matched the / at the start of the path in the request target.
  kPathStart,

  // Matched a forward slash in the path, including a trailing slash at the end
  // of the path.
  kPathSeparator,

  // Have reached the end of the path. There may be a query after the path, in
  // which case EPartialToken::kQueryString events will be reported.
  kPathEnd,

  // Matched "HTTP/1.1" at the end of the request's start line. No other version
  // is currently supported.
  kHttpVersion1_1,

  // All done decoding the headers.
  kHeadersEnd,
};

// For decoding events where we've have the whole string that makes up the named
// entity.
enum class EToken : uint_fast8_t {
  // The HTTP method's name (e.g. GET or DELETE). We don't support decoding of
  // HTTP methods that are longer than the largest string buffer allowed by the
  // caller of RequestDecoder.
  kHttpMethod,

  // A path segment is the the portion between two forward slashes in the path.
  kPathSegment,

  // The name of a header (e.g. "Content-Type");
  kHeaderName,

  // The value of a header (e.g. "text/plain").
  kHeaderValue,
};

// For decoding events where we've got some, and possibly all, of the value of
// the named entity in a StringView. This is restricted to the cases where the
// length of the token is greater than the maximum size of the input buffer
// passed to RequestDecoder::DecodeBuffer.
enum class EPartialToken : uint_fast8_t {
  // A path segment is the the portion between two forward slashes in the
  // path.
  kPathSegment,

  // The query string, i.e. following a request's path. We don't try to force
  // the buffering of the entire query string, so we don't have a corresponding
  // EToken enumerator. We make this choice because the query string requires a
  // further decoder.
  kQueryString,

  // The name of a header (e.g. "Content-Type");
  kHeaderName,

  // The value of a header (e.g. "text/plain").
  kHeaderValue,
};

enum class EPartialTokenPosition : uint_fast8_t {
  // Matched the start of an EPartialToken; e.g. saw a '?' at the end of a
  // path, where that '?' is not included in the query string passed to the
  // listener.
  kFirst,

  // Matched some text after the start of the token, and before the end. So far
  // this means that an OnPartialText notification with position kMiddle will
  // always have some text.
  kMiddle,

  // Matched the end of an EPartialToken; e.g. saw a '?' at the end of a
  // path, where that '?' is not included in the query string passed to the
  // listener.
  kLast
};

enum class EDecodeBufferStatus : uint_fast8_t {
  // All of the buffer has been decoded, and we've not yet reached the
  // end of the HTTP request message header (e.g. the \r\n\r\n at the end).
  kDecodingInProgress,

  // The current element being decoded starts after the beginning of the buffer
  // passed to DecodeBuffer, and extends to the end of the that buffer. At least
  // one byte will have been removed from the buffer. More input is required to
  // determine whether the entirety of that element will fit in a full buffer
  // (minus whatever trailing text is required to detect that we've got the
  // entire element), or if it must be treated as too big to be handled by our
  // small memory system.
  kNeedMoreInput,

  // We've reached the end of the message header.
  kComplete,

  // Before this enumerator the decoder is working fine, after this enumerator
  // the decoder has detected an error.
  kLastOkStatus = kComplete,

  // We've detected a fundamental error in the input (e.g. the request doesn't
  // start with an ALLCAPS word, such as GET or DELETE, following by a single
  // space). This can also be returned if the listener has requested that the
  // decoder enter an error state.
  kIllFormed,

  // Something has gone wrong, such as calling the decoder without clearing a
  // previously reported error, or if the buffer passed to DecodeBuffer is
  // empty.
  kInternalError,
};

// BEGIN_HEADER_GENERATED_BY_MAKE_ENUM_TO_STRING

const __FlashStringHelper* ToFlashStringHelper(EEvent v);
const __FlashStringHelper* ToFlashStringHelper(EToken v);
const __FlashStringHelper* ToFlashStringHelper(EPartialToken v);
const __FlashStringHelper* ToFlashStringHelper(EPartialTokenPosition v);
const __FlashStringHelper* ToFlashStringHelper(EDecodeBufferStatus v);

size_t PrintValueTo(EEvent v, Print& out);
size_t PrintValueTo(EToken v, Print& out);
size_t PrintValueTo(EPartialToken v, Print& out);
size_t PrintValueTo(EPartialTokenPosition v, Print& out);
size_t PrintValueTo(EDecodeBufferStatus v, Print& out);

#if MCU_HOST_TARGET
// Support for debug logging of enums.
std::ostream& operator<<(std::ostream& os, EEvent v);
std::ostream& operator<<(std::ostream& os, EToken v);
std::ostream& operator<<(std::ostream& os, EPartialToken v);
std::ostream& operator<<(std::ostream& os, EPartialTokenPosition v);
std::ostream& operator<<(std::ostream& os, EDecodeBufferStatus v);
#endif  // MCU_HOST_TARGET

// END_HEADER_GENERATED_BY_MAKE_ENUM_TO_STRING

}  // namespace http1
}  // namespace mcunet

#endif  // MCUNET_SRC_HTTP1_REQUEST_DECODER_CONSTANTS_H_
