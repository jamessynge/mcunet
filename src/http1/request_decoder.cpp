#include "http1/request_decoder.h"

#include <McuCore.h>
#include <ctype.h>  // pragma: keep standard include

#include "mcunet_config.h"

namespace mcunet {
namespace http1 {

using mcucore::StringView;

namespace mcunet_http1_internal {

bool IsOptionalWhitespace(const char c) { return c == ' ' || c == '\t'; }

// TODO(jamessynge): consider creating a class similar to std::bitset that can
// be used to implement IsPChar, IsQueryChar, etc., with an option to store the
// bytes in PROGMEM. Needs evaluation of the performance implications.

// Match characters allowed in a path segment.
MCU_CONSTEXPR_VAR StringView kExtraPChars("-._~!$&'()*+,;=%");
bool IsPChar(const char c) {
  return isAlphaNumeric(c) || kExtraPChars.contains(c);
}

// Match characters allowed in a query string.
bool IsQueryChar(const char c) { return IsPChar(c) || c == '/' || c == '?'; }

// Match characters allowed in a token (e.g. a header name).
MCU_CONSTEXPR_VAR StringView kExtraTChars("!#$%&'*+-.^_`|~");
bool IsTChar(const char c) {
  return isAlphaNumeric(c) || kExtraTChars.contains(c);
}

// Match characters allowed in a header value, per RFC7230, Section 3.2.6.
bool IsFieldContent(const char c) {
  return isPrintable(c) || c == '\t' || c >= 128;
}

// TODO(jamessynge): Move these Find(First|Last)NotOf functions to mcucore,
// e.g. as member functions of StringView, or as generic functions in
// string_compare.*.

// Return the index of the first character that doesn't match the test function.
// Returns StringView::kMaxSize if not found.
StringView::size_type FindFirstNotOf(const StringView& view,
                                     bool (*test)(char)) {
  for (StringView::size_type pos = 0; pos < view.size(); ++pos) {
    MCU_VLOG_VAR(9, pos) << MCU_NAME_VAL(view.at(pos))
                         << MCU_NAME_VAL((0 + view.at(pos)))
                         << MCU_NAME_VAL(test(view.at(pos)));
    if (!test(view.at(pos))) {
      return pos;
    }
  }
  return StringView::kMaxSize;
}

#if 0
// Find the last (highest index) character that doesn't match the test function.
// Returns kMaxSize if all of the characters match the test function, which is
// also true if the the view is empty.
StringView::size_type FindLastNotOf(const StringView& view,
                                    bool (*test)(char)) {
  for (StringView::size_type pos = view.size(); pos > 0;) {
    --pos;
    const char c = view.at(pos);
    if (!test(c)) {
      return pos;
    }
  }
  return StringView::kMaxSize;
}
#endif

// Removes leading whitespace characters, returns true when the first character
// is not whitespace.
bool SkipLeadingOptionalWhitespace(StringView& view) {
  const auto beyond = FindFirstNotOf(view, IsOptionalWhitespace);
  if (beyond == StringView::kMaxSize) {
    // They're all whitespace (or it is empty). Get rid of them. Choosing here
    // to treat this as a remove_prefix rather than a clear, so that tests see
    // that the pointer in the view is moved forward by the number of removed
    // characters.
    view.remove_prefix(view.size());
    // Since there are no characters in the view, we don't know if the next
    // input character will be a space or not, so we can't report true yet.
    return false;
  } else {
    view.remove_prefix(beyond);
    return true;
  }
}

// Remove optional whitespace from the end of the view, which is non-empty.
// Returns true if optional whitespace was removed.
bool TrimTrailingOptionalWhitespace(StringView& view) {
  MCU_DCHECK(!view.empty());
  bool result = false;
  do {
    if (!IsOptionalWhitespace(view.back())) {
      break;
    }
    view.remove_suffix(1);
    result = true;
  } while (!view.empty());
  return result;
}

}  // namespace mcunet_http1_internal

using namespace mcunet_http1_internal;  // NOLINT

using DecodeFunction = RequestDecoderImpl::DecodeFunction;
using StrSize = StringView::size_type;

// The goals of using ActiveDecodingState are to minimize the number of
// parameters passed to the DecodeFunctions, and to minimize the size (in bytes)
// of the RequestDecoderImpl, and hence of RequestDecoder. Both of these stem
// from trying to provide good performance on a low-memory system, where we
// might have multiple HTTP (TCP) connections active at once, but are operating
// in a single threaded fashion, so only one call to DecodeBuffer can be active
// at a time. So we can transiently use some stack space for ActiveDecodingState
// while decoding, but don't need any space for it after the call is complete.
// TODO(jamessynge): Are there any additional fields that we should add?
struct ActiveDecodingState {
  StringView input_buffer;
  RequestDecoderImpl& impl;
  DecodeFunction partial_decode_function_if_full{nullptr};
};

namespace {

using CharMatchFunction = bool (*)(char c);

// Using this macro to ensure that all declarations of functions passed to
// SetDecoderFunction are the same, and to make it easier to find all of those
// declarations when updating the PrintValue function for decoder functions.
#define DECODER_FUNCTION(function_name) \
  EDecodeBufferStatus function_name(ActiveDecodingState& state)

#define DECODER_ENTRY_CHECKS(state)            \
  MCU_DCHECK_LT(0, state.input_buffer.size()); \
  MCU_DCHECK_LT(state.input_buffer.size(), StringView::kMaxSize)

#define REPORT_ILLFORMED(state, msg_literal) \
  state.impl.OnIllFormed(MCU_PSD(msg_literal))

// We expect that the input buffer starts with the specified literal. If so,
// skip it and call matched_function, else report the specified error.
EDecodeBufferStatus SkipLiteralAndDispatch(
    ActiveDecodingState& state, const mcucore::ProgmemStringView& literal,
    DecodeFunction matched_function, mcucore::ProgmemString error_message) {
  if (mcucore::SkipPrefix(state.input_buffer, literal)) {
    // Yup, all done.
    return matched_function(state);
  } else if (mcucore::StartsWith(literal, state.input_buffer)) {
    // We've only got part of the literal.
    return EDecodeBufferStatus::kNeedMoreInput;
  } else {
    return state.impl.OnIllFormed(error_message);
  }
}

// Some token is too large to be decoded all in one go. All such methods are
// pretty much the same, so we provide this helper.
// TODO(jamessynge): Consider whether to store most of the params in a static
// (PROGMEM) struct.
EDecodeBufferStatus DecodePartialTokenHelper(ActiveDecodingState& state,
                                             CharMatchFunction is_token_matcher,
                                             EPartialToken token_type,
                                             DecodeFunction next_decoder) {
  DECODER_ENTRY_CHECKS(state);
  auto beyond = FindFirstNotOf(state.input_buffer, is_token_matcher);
  if (beyond == StringView::kMaxSize) {
    // The entirety of the buffer is part of the same, oversize token.
    const auto partial_token = state.input_buffer;
    state.input_buffer.remove_prefix(partial_token.size());
    state.impl.OnPartialText(token_type, EPartialTokenPosition::kMiddle,
                             partial_token);
  } else {
    // The first character beyond the current segment is in this buffer. It is
    // possible that it is the first character, but we don't have a
    // specialization for that case on the assumption that is relatively rare.
    const auto partial_token = state.input_buffer.prefix(beyond);
    state.input_buffer.remove_prefix(beyond);
    state.impl.SetDecodeFunction(next_decoder);
    state.impl.OnPartialText(token_type, EPartialTokenPosition::kLast,
                             partial_token);
  }
  return EDecodeBufferStatus::kDecodingInProgress;
}

// RequestDecoderImpl::DecodeBuffer has determined that the token can't fit
// into the largest buffer that its caller can provide, so we need to provide
// the token in pieces to the listener.
EDecodeBufferStatus DecodePartialTokenStartHelper(
    ActiveDecodingState& state, CharMatchFunction is_token_matcher,
    EPartialToken token_type, DecodeFunction remainder_decoder) {
  DECODER_ENTRY_CHECKS(state);
#ifndef NDEBUG
  MCU_DCHECK_EQ(FindFirstNotOf(state.input_buffer, is_token_matcher),
                StringView::kMaxSize);
#endif
  // The entirety of the buffer is part of the same, oversize token.
  const auto partial_token = state.input_buffer;
  state.input_buffer.remove_prefix(partial_token.size());
  state.impl.SetDecodeFunction(remainder_decoder);
  state.impl.OnPartialText(token_type, EPartialTokenPosition::kFirst,
                           partial_token);
  return EDecodeBufferStatus::kDecodingInProgress;
}

////////////////////////////////////////////////////////////////////////////////
// Decoder functions for different phases of decoding. Generally in reverse
// order to avoid forward declarations.

// Required forward declarations.
DECODER_FUNCTION(DecodeHeaderLines);
DECODER_FUNCTION(DecodePathSegment);

EDecodeBufferStatus MatchedEndOfHeaderLine(ActiveDecodingState& state) {
  state.impl.SetDecodeFunction(DecodeHeaderLines);
  return EDecodeBufferStatus::kDecodingInProgress;
}

DECODER_FUNCTION(MatchHeaderValueEnd) {
  DECODER_ENTRY_CHECKS(state);
  return SkipLiteralAndDispatch(state, MCU_PSV("\r\n"), MatchedEndOfHeaderLine,
                                MCU_PSD("Missing EOL after header"));
}

// The header value is apparently too long to fit in a buffer, so we pass
// whatever we can find in the input_buffer to the listener.
DECODER_FUNCTION(DecodePartialHeaderValue) {
  return DecodePartialTokenHelper(
      state, IsFieldContent, EPartialToken::kHeaderValue, MatchHeaderValueEnd);
}
DECODER_FUNCTION(DecodePartialHeaderValueStart) {
  return DecodePartialTokenStartHelper(state, IsFieldContent,
                                       EPartialToken::kHeaderValue,
                                       DecodePartialHeaderValue);
}

// Decode the part after the (field-name ":") on a header line.
DECODER_FUNCTION(DecodeHeaderValue) {
  DECODER_ENTRY_CHECKS(state);
  const auto beyond = FindFirstNotOf(state.input_buffer, IsFieldContent);
  if (0 < beyond && beyond < StringView::kMaxSize) {
    // We've got the entirety of a non-empty field value.
    auto value = state.input_buffer.prefix(beyond);
    state.input_buffer.remove_prefix(beyond);
    TrimTrailingOptionalWhitespace(value);
    state.impl.SetDecodeFunction(MatchHeaderValueEnd);
    state.impl.OnCompleteText(EToken::kHeaderValue, value);
    return EDecodeBufferStatus::kDecodingInProgress;
  } else if (beyond != 0) {
    // We didn't find the end of the name, so we need more input.
    state.partial_decode_function_if_full = DecodePartialHeaderValueStart;
    return EDecodeBufferStatus::kNeedMoreInput;
  }
  // It appears that the header value is empty, and that's bogus!
  return REPORT_ILLFORMED(state, "Empty header value");
}

// Before the header value, the user-agent may optionally insert whitespace.
DECODER_FUNCTION(SkipOptionalWhitespace) {
  DECODER_ENTRY_CHECKS(state);
  // The called method will remove any leading linear whitespace it finds, and
  // will return true if it has found the end of of such whitespace.
  if (SkipLeadingOptionalWhitespace(state.input_buffer)) {
    // Reached the end of the whitespace, if there was any.
    state.impl.SetDecodeFunction(DecodeHeaderValue);
  }
  return EDecodeBufferStatus::kDecodingInProgress;
}

DECODER_FUNCTION(MatchHeaderNameValueSeparator) {
  DECODER_ENTRY_CHECKS(state);
  if (state.input_buffer.starts_with(':')) {
    // Found the ':' separating the name from value.
    state.input_buffer.remove_prefix(1);
    state.impl.SetDecodeFunction(SkipOptionalWhitespace);
    return EDecodeBufferStatus::kDecodingInProgress;
  }
  return REPORT_ILLFORMED(state, "Expected colon after name");
}

// The header name is apparently too long to fit in a buffer, so we pass
// whatever we can find in the input_buffer to the listener.
DECODER_FUNCTION(DecodePartialHeaderName) {
  return DecodePartialTokenHelper(state, IsTChar, EPartialToken::kHeaderName,
                                  MatchHeaderNameValueSeparator);
}

DECODER_FUNCTION(DecodePartialHeaderNameStart) {
  return DecodePartialTokenStartHelper(
      state, IsTChar, EPartialToken::kHeaderName, DecodePartialHeaderName);
}

EDecodeBufferStatus MatchedEndOfHeaderLines(ActiveDecodingState& state) {
  state.impl.OnEnd();
  return EDecodeBufferStatus::kComplete;
}

// We're at the start of a header line, or at the end of the headers.
DECODER_FUNCTION(DecodeHeaderLines) {
  DECODER_ENTRY_CHECKS(state);
  const auto beyond = FindFirstNotOf(state.input_buffer, IsTChar);
  if (0 < beyond && beyond < StringView::kMaxSize) {
    // We've got a non-empty field name.
    const auto name = state.input_buffer.prefix(beyond);
    state.input_buffer.remove_prefix(beyond);
    state.impl.SetDecodeFunction(MatchHeaderNameValueSeparator);
    state.impl.OnCompleteText(EToken::kHeaderName, name);
    return EDecodeBufferStatus::kDecodingInProgress;
  } else if (beyond != 0) {
    // We didn't find the end of the name, so we need more input.
    state.partial_decode_function_if_full = DecodePartialHeaderNameStart;
    return EDecodeBufferStatus::kNeedMoreInput;
  }

  // No name, so we should be at the end of the headers.
  return SkipLiteralAndDispatch(state, MCU_PSV("\r\n"), MatchedEndOfHeaderLines,
                                MCU_PSD("Expected header name"));
}

EDecodeBufferStatus MatchedHttpVersion1_1(ActiveDecodingState& state) {
  state.impl.SetDecodeFunction(DecodeHeaderLines);
  state.impl.OnEvent(EEvent::kHttpVersion1_1);
  return EDecodeBufferStatus::kDecodingInProgress;
}

DECODER_FUNCTION(DecodeHttpVersion) {
  DECODER_ENTRY_CHECKS(state);
  return SkipLiteralAndDispatch(state, MCU_PSV("HTTP/1.1\r\n"),
                                MatchedHttpVersion1_1,
                                MCU_PSD("Unsupported HTTP version"));
}

// We've finished with the path and optional query string, so should be looking
// at a space followed by the HTTP version.
DECODER_FUNCTION(MatchAfterRequestTarget) {
  DECODER_ENTRY_CHECKS(state);
  const char c = state.input_buffer.at(0);
  if (c == ' ') {
    // End of the path, and there is no query.
    state.input_buffer.remove_prefix(1);
    state.impl.SetDecodeFunction(DecodeHttpVersion);
    return EDecodeBufferStatus::kDecodingInProgress;
  }
  MCU_DCHECK(!IsPChar(c)) << c;
  MCU_DCHECK(!IsQueryChar(c)) << c;
  return REPORT_ILLFORMED(state, "Invalid request target end");
}

DECODER_FUNCTION(DecodePartialQueryString) {
  return DecodePartialTokenHelper(
      state, IsQueryChar, EPartialToken::kQueryString, MatchAfterRequestTarget);
}

// We have a buffer whose contents immediately follow the "?" that marks the
// start of a query string. However, we don't choose to support OnCompleteText
// calls for the query string, because it can be quite variable in length, and
// it shouldn't matter to the listener whether it gets it all at once or in
// parts. Therefore, if we have any query chars, we pass them to the
// OnPartialText function. This is unlike the other DecodeXyzStart functions.
DECODER_FUNCTION(DecodeQueryStringStart) {
  DECODER_ENTRY_CHECKS(state);
  auto beyond = FindFirstNotOf(state.input_buffer, IsQueryChar);
  MCU_VLOG_VAR(1, beyond);
  if (beyond == 0) {
    // The query is empty, which is wierd but valid.
    state.impl.SetDecodeFunction(MatchAfterRequestTarget);
    state.impl.OnEvent(EEvent::kQueryAndUrlEnd);
    return EDecodeBufferStatus::kDecodingInProgress;
  }
  if (beyond == StringView::kMaxSize) {
    beyond = state.input_buffer.size();
  }
  const auto partial_query_string = state.input_buffer.prefix(beyond);
  state.input_buffer.remove_prefix(beyond);
  state.impl.SetDecodeFunction(DecodePartialQueryString);
  state.impl.OnPartialText(EPartialToken::kQueryString,
                           EPartialTokenPosition::kFirst, partial_query_string);
  return EDecodeBufferStatus::kDecodingInProgress;
}

// We've reached the end of valid path characters. Are we at the start of the
// query string, or and the end of the request target?
DECODER_FUNCTION(DecodeAfterPath) {
  DECODER_ENTRY_CHECKS(state);
  const char c = state.input_buffer.at(0);
  MCU_DCHECK(c != '/' && !IsPChar(c)) << c;
  if (c == '?') {
    // End of the path, start of the query.
    state.input_buffer.remove_prefix(1);
    state.impl.SetDecodeFunction(DecodeQueryStringStart);
    state.impl.OnEvent(EEvent::kPathEndQueryStart);
    return EDecodeBufferStatus::kDecodingInProgress;
  }
  // Appears to be the end of the request target (path and optional query).
  state.impl.SetDecodeFunction(MatchAfterRequestTarget);
  state.impl.OnEvent(EEvent::kPathAndUrlEnd);
  return EDecodeBufferStatus::kDecodingInProgress;
}

// We're decoding the path, have just finished decoding a path segment.
DECODER_FUNCTION(DecodeAfterSegment) {
  DECODER_ENTRY_CHECKS(state);
  const char c = state.input_buffer.at(0);
  MCU_DCHECK(!IsPChar(c)) << c;
  if (c == '/') {
    // Maybe there is another segment in the path.
    state.input_buffer.remove_prefix(1);
    state.impl.SetDecodeFunction(DecodePathSegment);
    state.impl.OnEvent(EEvent::kPathSeparator);
    return EDecodeBufferStatus::kDecodingInProgress;
  } else {
    return DecodeAfterPath(state);
  }
}

// The path segment is apparently too long to fit in a buffer, so we pass
// whatever we can find in the input_buffer to the listener.
DECODER_FUNCTION(DecodePartialPathSegment) {
  return DecodePartialTokenHelper(state, IsPChar, EPartialToken::kPathSegment,
                                  DecodeAfterSegment);
}

// The path segment is apparently too long to fit in a buffer, so we pass
// whatever we can find in the input_buffer to the listener.
DECODER_FUNCTION(DecodePartialPathSegmentStart) {
  return DecodePartialTokenStartHelper(
      state, IsPChar, EPartialToken::kPathSegment, DecodePartialPathSegment);
}

// The previous character matched is a '/'. We're either at the beginning of a
// path segment, or at the end of the (valid) path.
DECODER_FUNCTION(DecodePathSegment) {
  DECODER_ENTRY_CHECKS(state);
  const auto beyond = FindFirstNotOf(state.input_buffer, IsPChar);
  if (0 < beyond && beyond < StringView::kMaxSize) {
    // We've got a non-empty segment.
    const auto segment = state.input_buffer.prefix(beyond);
    state.input_buffer.remove_prefix(beyond);
    state.impl.SetDecodeFunction(DecodeAfterSegment);
    state.impl.OnCompleteText(EToken::kPathSegment, segment);
    return EDecodeBufferStatus::kDecodingInProgress;
  } else if (beyond == 0) {
    return DecodeAfterPath(state);
  } else {
    // We didn't find the end of the segment, so we need more input.
    state.partial_decode_function_if_full = DecodePartialPathSegmentStart;
    return EDecodeBufferStatus::kNeedMoreInput;
  }
}

DECODER_FUNCTION(DecodeStartOfPath) {
  DECODER_ENTRY_CHECKS(state);
  if (state.input_buffer.starts_with('/')) {
    // We're decoding an origin-form request-target.
    // https://datatracker.ietf.org/doc/html/rfc7230#section-5.3.1
    state.input_buffer.remove_prefix(1);
    state.impl.SetDecodeFunction(DecodePathSegment);
    state.impl.OnEvent(EEvent::kPathStart);
    return EDecodeBufferStatus::kDecodingInProgress;
  }
  return REPORT_ILLFORMED(state, "Invalid path start");
}

DECODER_FUNCTION(DecodeHttpMethodError) {
  return REPORT_ILLFORMED(state, "HTTP Method too long");
}

// Decode an HTTP method. If definitely not present, reports an error.
DECODER_FUNCTION(DecodeHttpMethod) {
  DECODER_ENTRY_CHECKS(state);

  // https://www.iana.org/assignments/http-methods/http-methods.xhtml contains
  // the registered HTTP methods. All names are uppercase ASCII, and so far
  // those are all we're likely to care about, so those are all that I'm
  // attempting to support here. Note that "*" is also reserved, though in
  // that case to prevent it being used as a method name; for more info, see:
  // https://www.rfc-editor.org/rfc/rfc9110.html#name-method-registration

  if (isupper(state.input_buffer.at(0))) {
    // Starts with uppercase, so we know that the method could be present.

    for (StrSize pos = 1; pos < state.input_buffer.size(); ++pos) {
      const char c = state.input_buffer.at(pos);
      if (isupper(c)) {
        continue;
      } else if (c == ' ') {
        // Reached the expected end of the method.
        auto text = state.input_buffer.prefix(pos);
        state.input_buffer.remove_prefix(pos + 1);
        state.impl.SetDecodeFunction(DecodeStartOfPath);
        state.impl.OnCompleteText(EToken::kHttpMethod, text);
        return EDecodeBufferStatus::kDecodingInProgress;
      } else {
        return REPORT_ILLFORMED(state, "Invalid HTTP method end");
      }
    }

    // Haven't found the end of the method. Caller will need to figure out if
    // more input is possible.
    state.partial_decode_function_if_full = DecodeHttpMethodError;
    return EDecodeBufferStatus::kNeedMoreInput;
  }
  return REPORT_ILLFORMED(state, "Invalid HTTP method start");
}

DECODER_FUNCTION(DecodeError) {
#ifdef MCU_REQUEST_DECODER_EXTRA_CHECKS
  MCU_VLOG(2) << MCU_PSD("RequestDecoder is not ready for decoding");
#endif  // MCU_REQUEST_DECODER_EXTRA_CHECKS
  return EDecodeBufferStatus::kInternalError;
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////

size_t PrintValueTo(DecodeFunction decode_function, Print& out) {
#define OUTPUT_METHOD_NAME(symbol)                                 \
  if (decode_function == symbol) {                                 \
    return mcucore::PrintProgmemStringData(MCU_PSD(#symbol), out); \
  }
  /*
Command for generating the 'table' below:

egrep "^DECODER_FUNCTION\(\w+\) \{" mcunet/src/http1/request_decoder.cc | \
  cut '-d ' -f1 | \
  sed -e "s/DECODER_FUNCTION/  OUTPUT_METHOD_NAME/ ; s/$/;/" | \
  sort

  */

  OUTPUT_METHOD_NAME(DecodeAfterPath);
  OUTPUT_METHOD_NAME(DecodeAfterSegment);
  OUTPUT_METHOD_NAME(DecodeError);
  OUTPUT_METHOD_NAME(DecodeHeaderLines);
  OUTPUT_METHOD_NAME(DecodeHeaderValue);
  OUTPUT_METHOD_NAME(DecodeHttpMethod);
  OUTPUT_METHOD_NAME(DecodeHttpMethodError);
  OUTPUT_METHOD_NAME(DecodeHttpVersion);
  OUTPUT_METHOD_NAME(DecodePartialHeaderName);
  OUTPUT_METHOD_NAME(DecodePartialHeaderNameStart);
  OUTPUT_METHOD_NAME(DecodePartialHeaderValue);
  OUTPUT_METHOD_NAME(DecodePartialHeaderValueStart);
  OUTPUT_METHOD_NAME(DecodePartialPathSegment);
  OUTPUT_METHOD_NAME(DecodePartialPathSegmentStart);
  OUTPUT_METHOD_NAME(DecodePartialQueryString);
  OUTPUT_METHOD_NAME(DecodePathSegment);
  OUTPUT_METHOD_NAME(DecodeQueryStringStart);
  OUTPUT_METHOD_NAME(DecodeStartOfPath);
  OUTPUT_METHOD_NAME(MatchAfterRequestTarget);
  OUTPUT_METHOD_NAME(MatchHeaderNameValueSeparator);
  OUTPUT_METHOD_NAME(MatchHeaderValueEnd);
  OUTPUT_METHOD_NAME(SkipOptionalWhitespace);

#undef OUTPUT_METHOD_NAME

  MCU_CHECK(false) << MCU_FLASHSTR(                       // COV_NF_LINE
      "Haven't implemented a case for decode_function");  // COV_NF_LINE
  return 0;                                               // COV_NF_LINE
}

RequestDecoderImpl::RequestDecoderImpl() { decode_function_ = DecodeError; }

void RequestDecoderImpl::Reset() {
  decode_function_ = DecodeHttpMethod;
  ClearListener();
}

void RequestDecoderImpl::SetListener(RequestDecoderListener& listener) {
  listener_ = &listener;
}

void RequestDecoderImpl::ClearListener() { listener_ = nullptr; }

void RequestDecoderImpl::SetDecodeFunction(DecodeFunction decode_function) {
  MCU_VLOG(2) << MCU_PSD("Set") << MCU_NAME_VAL(decode_function);
  decode_function_ = decode_function;
}

EDecodeBufferStatus RequestDecoderImpl::DecodeBuffer(
    StringView& buffer, const bool buffer_is_full) {
  MCU_VLOG(1) << MCU_PSD("ENTER DecodeBuffer size=") << buffer.size()
              << MCU_NAME_VAL(buffer_is_full);

  MCU_DCHECK_LT(0, buffer.size());
  MCU_DCHECK_LT(buffer.size(), StringView::kMaxSize);

  MCU_CHECK_NE(decode_function_, nullptr);

  const auto start_size = buffer.size();

  ActiveDecodingState active_state{.input_buffer = buffer, .impl = *this};
  auto status = EDecodeBufferStatus::kNeedMoreInput;
  while (!active_state.input_buffer.empty()) {
    const auto buffer_size_before_decode = active_state.input_buffer.size();
#ifdef MCU_REQUEST_DECODER_EXTRA_CHECKS
    const auto old_decode_function = decode_function_;
    MCU_VLOG(2) << MCU_PSD("Passing ") << decode_function_
                << MCU_PSD(" buffer ")
                << mcucore::HexEscaped(active_state.input_buffer)
                << MCU_PSD(" (")
                << static_cast<size_t>(active_state.input_buffer.size())
                << MCU_PSD(" chars)");
#endif  // MCU_REQUEST_DECODER_EXTRA_CHECKS

    status = decode_function_(active_state);
    const auto consumed_chars =
        buffer_size_before_decode - active_state.input_buffer.size();

#ifdef MCU_REQUEST_DECODER_EXTRA_CHECKS
    MCU_VLOG(2) << MCU_PSD("decode_function_") << MCU_PSD(" returned")
                << MCU_NAME_VAL(status) << MCU_NAME_VAL(consumed_chars);
    MCU_CHECK_LE(active_state.input_buffer.size(), buffer_size_before_decode);
    MCU_VLOG(3) << MCU_PSD("decode_function_")
                << (old_decode_function == decode_function_
                        ? MCU_PSV(" unchanged")
                        : MCU_PSV(" changed"));
#endif  // MCU_REQUEST_DECODER_EXTRA_CHECKS

    if (status == EDecodeBufferStatus::kDecodingInProgress) {
      // This is expected to be the most common status, so we check it first.
#ifdef MCU_REQUEST_DECODER_EXTRA_CHECKS
      MCU_DCHECK(
          !(consumed_chars == 0 && old_decode_function == decode_function_))
          << MCU_PSD("decode_function_")       // COV_NF_LINE
          << MCU_PSD(" should have changed");  // COV_NF_LINE
#endif  // MCU_REQUEST_DECODER_EXTRA_CHECKS
      continue;
    } else if (status == EDecodeBufferStatus::kNeedMoreInput) {
      // The the decode function requires more data to locate the end of the
      // entity it is attempting to decode.
      MCU_DCHECK(!active_state.input_buffer.empty());
      if (buffer_is_full && consumed_chars == 0 &&
          active_state.input_buffer.size() == start_size) {
        // The caller won't be able to provide more because the buffer is as
        // full as it can get, so we either need a fallback decoder, or we have
        // to report an error.
        if (active_state.partial_decode_function_if_full != nullptr) {
          decode_function_ = active_state.partial_decode_function_if_full;
          active_state.partial_decode_function_if_full = nullptr;
          continue;
        }
        return EDecodeBufferStatus::kIllFormed;  // COV_NF_LINE
      }
      // Ask the caller to provide more input.
      break;
    } else {
      // All done decoding, either because we've succeeded in decoding the
      // entire request header, the header is malformed, or we encountered an
      // error.
      break;
    }
  }
  buffer = active_state.input_buffer;
  MCU_VLOG(1) << MCU_PSD("EXIT DecodeBuffer after consuming ")
              << (start_size - buffer.size()) << MCU_PSD(" characters");
  return status;
}

// Event delivery methods. These allow us to log centrally, and to deal with
// an unset listener centrally.
void RequestDecoderImpl::OnEvent(EEvent event) {
  MCU_VLOG(3) << "==>> OnEvent " << event;
  if (listener_ != nullptr) {
    listener_->OnEvent(event);
  }
}

void RequestDecoderImpl::OnCompleteText(EToken token, StringView text) {
  MCU_VLOG(3) << "==>> OnCompleteText " << token << MCU_PSD(", ")
              << mcucore::HexEscaped(text);
  if (listener_ != nullptr) {
    listener_->OnCompleteText(token, text);
  }
}

void RequestDecoderImpl::OnPartialText(EPartialToken token,
                                       EPartialTokenPosition position,
                                       StringView text) {
  MCU_VLOG(3) << "==>> OnPartialText " << token << MCU_PSD(", ") << position
              << MCU_PSD(", ") << mcucore::HexEscaped(text);
  if (listener_ != nullptr) {
    listener_->OnPartialText(token, position, text);
  }
}

void RequestDecoderImpl::OnEnd() {
  MCU_VLOG(3) << "==>> OnEnd";
  SetDecodeFunction(DecodeError);
  if (listener_ != nullptr) {
    listener_->OnEnd();
  }
}

EDecodeBufferStatus RequestDecoderImpl::OnIllFormed(
    mcucore::ProgmemString msg) {
  MCU_VLOG(3) << "==>> OnIllFormed " << msg;
  SetDecodeFunction(DecodeError);
  if (listener_ != nullptr) {
    listener_->OnError(msg);
  }
  return EDecodeBufferStatus::kIllFormed;
}

}  // namespace http1
}  // namespace mcunet