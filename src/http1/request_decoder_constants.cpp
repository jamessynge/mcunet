#include "http1/request_decoder_constants.h"

#if MCU_HOST_TARGET
#include <ostream>  // pragma: keep standard include
#endif

#include <McuCore.h>

namespace mcunet {
namespace http1 {

// BEGIN_SOURCE_GENERATED_BY_MAKE_ENUM_TO_STRING

const __FlashStringHelper* ToFlashStringHelper(EEvent v) {
#ifdef TO_FLASH_STRING_HELPER_USE_SWITCH
  switch (v) {
    case EEvent::kPathStart:
      return MCU_FLASHSTR("PathStart");
    case EEvent::kPathSeparator:
      return MCU_FLASHSTR("PathSeparator");
    case EEvent::kPathAndUrlEnd:
      return MCU_FLASHSTR("PathAndUrlEnd");
    case EEvent::kPathEndQueryStart:
      return MCU_FLASHSTR("PathEndQueryStart");
    case EEvent::kQueryAndUrlEnd:
      return MCU_FLASHSTR("QueryAndUrlEnd");
    case EEvent::kHttpVersion1_1:
      return MCU_FLASHSTR("HttpVersion1_1");
  }
  return nullptr;
#elif defined(TO_FLASH_STRING_HELPER_PREFER_IF_STATEMENTS)
  if (v == EEvent::kPathStart) {
    return MCU_FLASHSTR("PathStart");
  }
  if (v == EEvent::kPathSeparator) {
    return MCU_FLASHSTR("PathSeparator");
  }
  if (v == EEvent::kPathAndUrlEnd) {
    return MCU_FLASHSTR("PathAndUrlEnd");
  }
  if (v == EEvent::kPathEndQueryStart) {
    return MCU_FLASHSTR("PathEndQueryStart");
  }
  if (v == EEvent::kQueryAndUrlEnd) {
    return MCU_FLASHSTR("QueryAndUrlEnd");
  }
  if (v == EEvent::kHttpVersion1_1) {
    return MCU_FLASHSTR("HttpVersion1_1");
  }
  return nullptr;
#else   // Use flash string table.
  static MCU_FLASH_STRING_TABLE(
      flash_string_table,
      MCU_FLASHSTR("PathStart"),          // 0: kPathStart
      MCU_FLASHSTR("PathSeparator"),      // 1: kPathSeparator
      MCU_FLASHSTR("PathAndUrlEnd"),      // 2: kPathAndUrlEnd
      MCU_FLASHSTR("PathEndQueryStart"),  // 3: kPathEndQueryStart
      MCU_FLASHSTR("QueryAndUrlEnd"),     // 4: kQueryAndUrlEnd
      MCU_FLASHSTR("HttpVersion1_1"),     // 5: kHttpVersion1_1
  );
  return mcucore::LookupFlashStringForDenseEnum<uint_fast8_t>(
      flash_string_table, EEvent::kPathStart, EEvent::kHttpVersion1_1, v);
#endif  // TO_FLASH_STRING_HELPER_USE_SWITCH
}

const __FlashStringHelper* ToFlashStringHelper(EToken v) {
#ifdef TO_FLASH_STRING_HELPER_USE_SWITCH
  switch (v) {
    case EToken::kHttpMethod:
      return MCU_FLASHSTR("HttpMethod");
    case EToken::kPathSegment:
      return MCU_FLASHSTR("PathSegment");
    case EToken::kHeaderName:
      return MCU_FLASHSTR("HeaderName");
    case EToken::kHeaderValue:
      return MCU_FLASHSTR("HeaderValue");
  }
  return nullptr;
#elif defined(TO_FLASH_STRING_HELPER_PREFER_IF_STATEMENTS)
  if (v == EToken::kHttpMethod) {
    return MCU_FLASHSTR("HttpMethod");
  }
  if (v == EToken::kPathSegment) {
    return MCU_FLASHSTR("PathSegment");
  }
  if (v == EToken::kHeaderName) {
    return MCU_FLASHSTR("HeaderName");
  }
  if (v == EToken::kHeaderValue) {
    return MCU_FLASHSTR("HeaderValue");
  }
  return nullptr;
#else   // Use flash string table.
  static MCU_FLASH_STRING_TABLE(flash_string_table,
                                MCU_FLASHSTR("HttpMethod"),   // 0: kHttpMethod
                                MCU_FLASHSTR("PathSegment"),  // 1: kPathSegment
                                MCU_FLASHSTR("HeaderName"),   // 2: kHeaderName
                                MCU_FLASHSTR("HeaderValue"),  // 3: kHeaderValue
  );
  return mcucore::LookupFlashStringForDenseEnum<uint_fast8_t>(
      flash_string_table, EToken::kHttpMethod, EToken::kHeaderValue, v);
#endif  // TO_FLASH_STRING_HELPER_USE_SWITCH
}

const __FlashStringHelper* ToFlashStringHelper(EPartialToken v) {
#ifdef TO_FLASH_STRING_HELPER_USE_SWITCH
  switch (v) {
    case EPartialToken::kPathSegment:
      return MCU_FLASHSTR("PathSegment");
    case EPartialToken::kQueryString:
      return MCU_FLASHSTR("QueryString");
    case EPartialToken::kHeaderName:
      return MCU_FLASHSTR("HeaderName");
    case EPartialToken::kHeaderValue:
      return MCU_FLASHSTR("HeaderValue");
  }
  return nullptr;
#elif defined(TO_FLASH_STRING_HELPER_PREFER_IF_STATEMENTS)
  if (v == EPartialToken::kPathSegment) {
    return MCU_FLASHSTR("PathSegment");
  }
  if (v == EPartialToken::kQueryString) {
    return MCU_FLASHSTR("QueryString");
  }
  if (v == EPartialToken::kHeaderName) {
    return MCU_FLASHSTR("HeaderName");
  }
  if (v == EPartialToken::kHeaderValue) {
    return MCU_FLASHSTR("HeaderValue");
  }
  return nullptr;
#else   // Use flash string table.
  static MCU_FLASH_STRING_TABLE(flash_string_table,
                                MCU_FLASHSTR("PathSegment"),  // 0: kPathSegment
                                MCU_FLASHSTR("QueryString"),  // 1: kQueryString
                                MCU_FLASHSTR("HeaderName"),   // 2: kHeaderName
                                MCU_FLASHSTR("HeaderValue"),  // 3: kHeaderValue
  );
  return mcucore::LookupFlashStringForDenseEnum<uint_fast8_t>(
      flash_string_table, EPartialToken::kPathSegment,
      EPartialToken::kHeaderValue, v);
#endif  // TO_FLASH_STRING_HELPER_USE_SWITCH
}

const __FlashStringHelper* ToFlashStringHelper(EPartialTokenPosition v) {
#ifdef TO_FLASH_STRING_HELPER_USE_SWITCH
  switch (v) {
    case EPartialTokenPosition::kFirst:
      return MCU_FLASHSTR("First");
    case EPartialTokenPosition::kMiddle:
      return MCU_FLASHSTR("Middle");
    case EPartialTokenPosition::kLast:
      return MCU_FLASHSTR("Last");
  }
  return nullptr;
#elif defined(TO_FLASH_STRING_HELPER_PREFER_IF_STATEMENTS)
  if (v == EPartialTokenPosition::kFirst) {
    return MCU_FLASHSTR("First");
  }
  if (v == EPartialTokenPosition::kMiddle) {
    return MCU_FLASHSTR("Middle");
  }
  if (v == EPartialTokenPosition::kLast) {
    return MCU_FLASHSTR("Last");
  }
  return nullptr;
#else   // Use flash string table.
  static MCU_FLASH_STRING_TABLE(flash_string_table,
                                MCU_FLASHSTR("First"),   // 0: kFirst
                                MCU_FLASHSTR("Middle"),  // 1: kMiddle
                                MCU_FLASHSTR("Last"),    // 2: kLast
  );
  return mcucore::LookupFlashStringForDenseEnum<uint_fast8_t>(
      flash_string_table, EPartialTokenPosition::kFirst,
      EPartialTokenPosition::kLast, v);
#endif  // TO_FLASH_STRING_HELPER_USE_SWITCH
}

const __FlashStringHelper* ToFlashStringHelper(EDecodeBufferStatus v) {
#ifdef TO_FLASH_STRING_HELPER_USE_SWITCH
  switch (v) {
    case EDecodeBufferStatus::kDecodingInProgress:
      return MCU_FLASHSTR("DecodingInProgress");
    case EDecodeBufferStatus::kNeedMoreInput:
      return MCU_FLASHSTR("NeedMoreInput");
    case EDecodeBufferStatus::kComplete:
      return MCU_FLASHSTR("Complete");
    case EDecodeBufferStatus::kLastOkStatus:
      return MCU_FLASHSTR("LastOkStatus");
    case EDecodeBufferStatus::kIllFormed:
      return MCU_FLASHSTR("IllFormed");
    case EDecodeBufferStatus::kInternalError:
      return MCU_FLASHSTR("InternalError");
  }
  return nullptr;
#else   // Use if statements.
  if (v == EDecodeBufferStatus::kDecodingInProgress) {
    return MCU_FLASHSTR("DecodingInProgress");
  }
  if (v == EDecodeBufferStatus::kNeedMoreInput) {
    return MCU_FLASHSTR("NeedMoreInput");
  }
  if (v == EDecodeBufferStatus::kComplete) {
    return MCU_FLASHSTR("Complete");
  }
  if (v == EDecodeBufferStatus::kLastOkStatus) {
    return MCU_FLASHSTR("LastOkStatus");
  }
  if (v == EDecodeBufferStatus::kIllFormed) {
    return MCU_FLASHSTR("IllFormed");
  }
  if (v == EDecodeBufferStatus::kInternalError) {
    return MCU_FLASHSTR("InternalError");
  }
  return nullptr;
#endif  // TO_FLASH_STRING_HELPER_USE_SWITCH
}

size_t PrintValueTo(EEvent v, Print& out) {
  auto flash_string = ToFlashStringHelper(v);
  if (flash_string != nullptr) {
    return out.print(flash_string);
  }
  return mcucore::PrintUnknownEnumValueTo(MCU_FLASHSTR("EEvent"),
                                          static_cast<uint32_t>(v), out);
}

size_t PrintValueTo(EToken v, Print& out) {
  auto flash_string = ToFlashStringHelper(v);
  if (flash_string != nullptr) {
    return out.print(flash_string);
  }
  return mcucore::PrintUnknownEnumValueTo(MCU_FLASHSTR("EToken"),
                                          static_cast<uint32_t>(v), out);
}

size_t PrintValueTo(EPartialToken v, Print& out) {
  auto flash_string = ToFlashStringHelper(v);
  if (flash_string != nullptr) {
    return out.print(flash_string);
  }
  return mcucore::PrintUnknownEnumValueTo(MCU_FLASHSTR("EPartialToken"),
                                          static_cast<uint32_t>(v), out);
}

size_t PrintValueTo(EPartialTokenPosition v, Print& out) {
  auto flash_string = ToFlashStringHelper(v);
  if (flash_string != nullptr) {
    return out.print(flash_string);
  }
  return mcucore::PrintUnknownEnumValueTo(MCU_FLASHSTR("EPartialTokenPosition"),
                                          static_cast<uint32_t>(v), out);
}

size_t PrintValueTo(EDecodeBufferStatus v, Print& out) {
  auto flash_string = ToFlashStringHelper(v);
  if (flash_string != nullptr) {
    return out.print(flash_string);
  }
  return mcucore::PrintUnknownEnumValueTo(MCU_FLASHSTR("EDecodeBufferStatus"),
                                          static_cast<uint32_t>(v), out);
}

#if MCU_HOST_TARGET
// Support for debug logging of enums.

std::ostream& operator<<(std::ostream& os, EEvent v) {
  switch (v) {
    case EEvent::kPathStart:
      return os << "PathStart";
    case EEvent::kPathSeparator:
      return os << "PathSeparator";
    case EEvent::kPathAndUrlEnd:
      return os << "PathAndUrlEnd";
    case EEvent::kPathEndQueryStart:
      return os << "PathEndQueryStart";
    case EEvent::kQueryAndUrlEnd:
      return os << "QueryAndUrlEnd";
    case EEvent::kHttpVersion1_1:
      return os << "HttpVersion1_1";
  }
  return os << "Unknown EEvent, value=" << static_cast<int64_t>(v);
}

std::ostream& operator<<(std::ostream& os, EToken v) {
  switch (v) {
    case EToken::kHttpMethod:
      return os << "HttpMethod";
    case EToken::kPathSegment:
      return os << "PathSegment";
    case EToken::kHeaderName:
      return os << "HeaderName";
    case EToken::kHeaderValue:
      return os << "HeaderValue";
  }
  return os << "Unknown EToken, value=" << static_cast<int64_t>(v);
}

std::ostream& operator<<(std::ostream& os, EPartialToken v) {
  switch (v) {
    case EPartialToken::kPathSegment:
      return os << "PathSegment";
    case EPartialToken::kQueryString:
      return os << "QueryString";
    case EPartialToken::kHeaderName:
      return os << "HeaderName";
    case EPartialToken::kHeaderValue:
      return os << "HeaderValue";
  }
  return os << "Unknown EPartialToken, value=" << static_cast<int64_t>(v);
}

std::ostream& operator<<(std::ostream& os, EPartialTokenPosition v) {
  switch (v) {
    case EPartialTokenPosition::kFirst:
      return os << "First";
    case EPartialTokenPosition::kMiddle:
      return os << "Middle";
    case EPartialTokenPosition::kLast:
      return os << "Last";
  }
  return os << "Unknown EPartialTokenPosition, value="
            << static_cast<int64_t>(v);
}

std::ostream& operator<<(std::ostream& os, EDecodeBufferStatus v) {
  switch (v) {
    case EDecodeBufferStatus::kDecodingInProgress:
      return os << "DecodingInProgress";
    case EDecodeBufferStatus::kNeedMoreInput:
      return os << "NeedMoreInput";
    case EDecodeBufferStatus::kComplete:
      return os << "Complete";
    case EDecodeBufferStatus::kIllFormed:
      return os << "IllFormed";
    case EDecodeBufferStatus::kInternalError:
      return os << "InternalError";
  }
  return os << "Unknown EDecodeBufferStatus, value=" << static_cast<int64_t>(v);
}

#endif  // MCU_HOST_TARGET

// END_SOURCE_GENERATED_BY_MAKE_ENUM_TO_STRING

}  // namespace http1
}  // namespace mcunet
