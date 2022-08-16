#include "http1/request_decoder.h"

#include <McuCore.h>
#include <ctype.h>  // pragma: keep standard include

#include "http1/request_decoder_constants.h"

// TODO(jamessynge): See if we can abstract the handling of entities where we
// use FindFirstNotOf(IsXChar), where IsXChar excludes some characters that are
// valid in the entity, such as % and + in query string parameter names. These
// excluded characters need some special handling.

namespace mcunet {
namespace http1 {

using mcucore::StringView;

// Using this macro to ensure that all declarations of functions passed to
// SetDecoderFunction are the same, and to make it easier to find all of those
// declarations when updating the PrintValue function for decoder functions.
#define DECODER_FUNCTION(function_name) \
  EDecodeBufferStatus function_name(ActiveDecodingState& state)

namespace {
// Forward decl to make life easier.
DECODER_FUNCTION(DecodeInternalError);
}  // namespace

namespace mcunet_http1_internal {

bool IsOptionalWhitespace(const char c) { return c == ' ' || c == '\t'; }

// Match characters allowed BY THIS DECODER in a method name.
// https://www.iana.org/assignments/http-methods/http-methods.xhtml contains the
// registered HTTP methods. The names are uppercase ASCII, two with a hyphen
// separating words in the name, and those registered names all we're likely to
// care about, so those are all that I'm attempting to support here. Note that
// "*" is also in the registry, though in that case to document that it is
// reserved, and must not be used as a method name; for more info, see:
// https://www.rfc-editor.org/rfc/rfc9110.html#name-method-registration
bool IsMethodChar(const char c) { return isUpperCase(c) || c == '-'; }

// Match characters allowed in a path segment, excluding percent, which is
// handled separately.
bool IsPCharExceptPercent(const char c) {
  // The MCU_PSV string is in ASCII order.
  return isAlphaNumeric(c) || MCU_PSV("!$&'()*+,-.;=_~").contains(c);
}

// Match characters allowed in a path segment.
bool IsPChar(const char c) {
  // The MCU_PSV string is in ASCII order.
  return IsPCharExceptPercent(c) || c == '%';
}

// Match characters allowed in a query string.
bool IsQueryChar(const char c) {
  // The MCU_PSV string is in ASCII order.
  return isAlphaNumeric(c) || MCU_PSV("!$%&'()*+,-./;=?_~").contains(c);
}

// Match characters allowed in the parameter name of a query string, excluding
// percent and plus, which are handled separately.
bool IsParamNameCharExceptPercentAndPlus(const char c) {
  // The MCU_PSV string is in ASCII order.
  return isAlphaNumeric(c) || MCU_PSV("!$'()*,-./;?_~").contains(c);
}

// Match characters allowed in the parameter value of a query string.
bool IsParamValueCharExceptPercentAndPlus(const char c) {
  // The MCU_PSV string is in ASCII order.
  return isAlphaNumeric(c) || MCU_PSV("!$'()*,-./;=?_~").contains(c);
}

// Match characters allowed in a token (e.g. a header name).
bool IsTokenChar(const char c) {
  // The MCU_PSV string is in ASCII order.
  return isAlphaNumeric(c) || MCU_PSV("!#$%&'*+-.^_`|~").contains(c);
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
//
// NOTE: Given that the IsXyzChar functions passed here consist primarily of a
// call to a ctype.h function with a fallback to a check for presence in a
// string, we could consider using memspn/memspn_P for the latter. This will
// require some benchmarking to determine if it has value.
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
// at a time. We can transiently use some stack space for ActiveDecodingState
// while decoding, but don't need any space for it after the call is complete.
// TODO(jamessynge): Are there any additional fields that we should add?
struct ActiveDecodingState {
  ActiveDecodingState(const StringView& full_decoder_input,
                      RequestDecoderListener& listener,
                      RequestDecoderImpl& impl)
      : input_buffer(full_decoder_input),
        listener(listener),
        impl(impl),
        full_decoder_input(full_decoder_input),
        base_data(*this) {}

  void StopDecoding() const { SetDecodeFunction(DecodeInternalError); }

  void SetDecodeFunction(DecodeFunction decode_function) const {
    MCU_VLOG(2) << MCU_PSD("Set") << MCU_NAME_VAL(decode_function);
    impl.decode_function_ = decode_function;
  }

  DecodeFunction GetDecodeFunction() const { return impl.decode_function_; }

  // Event delivery methods. These allow us to log centrally, and to deal with
  // an unset listener centrally.

  void OnEvent(EEvent event) {
    MCU_VLOG(3) << "==>> OnEvent " << event;
    on_event_data.event = event;
    listener.OnEvent(on_event_data);
  }

  void OnCompleteText(EToken token, StringView text) {
    MCU_VLOG(3) << "==>> OnCompleteText " << token << MCU_PSD(", ")
                << mcucore::HexEscaped(text);
    on_complete_text_data.token = token;
    on_complete_text_data.text = text;
    listener.OnCompleteText(on_complete_text_data);
  }

  void OnPartialText(EPartialToken token, EPartialTokenPosition position,
                     StringView text) {
    MCU_VLOG(3) << "==>> OnPartialText " << token << MCU_PSD(", ") << position
                << MCU_PSD(", ") << mcucore::HexEscaped(text);
    on_partial_text_data.token = token;
    on_partial_text_data.position = position;
    on_partial_text_data.text = text;
    listener.OnPartialText(on_partial_text_data);
  }

  void OnHeadersEnd() {
    MCU_VLOG(3) << "==>> OnHeadersEnd";
    SetDecodeFunction(DecodeInternalError);
    on_event_data.event = EEvent::kHeadersEnd;
    listener.OnEvent(on_event_data);
  }

  EDecodeBufferStatus OnIllFormed(mcucore::ProgmemString message) {
    MCU_VLOG(3) << "==>> OnIllFormed " << message;
    SetDecodeFunction(DecodeInternalError);
    on_error_data.message = message;
    on_error_data.undecoded_input = input_buffer;
    listener.OnError(on_error_data);
    return EDecodeBufferStatus::kIllFormed;
  }

  StringView input_buffer;
  RequestDecoderListener& listener;
  RequestDecoderImpl& impl;
  DecodeFunction partial_decode_function_if_full{nullptr};
  const StringView full_decoder_input;

  union {
    BaseListenerCallbackData base_data;
    OnEventData on_event_data;
    OnCompleteTextData on_complete_text_data;
    OnPartialTextData on_partial_text_data;
    OnErrorData on_error_data;
  };
};

void BaseListenerCallbackData::StopDecoding() const { state.StopDecoding(); }

mcucore::StringView BaseListenerCallbackData::GetFullDecoderInput() const {
  return state.full_decoder_input;
}

namespace {
using CharMatchFunction = bool (*)(char c);

bool HexCharToNibble(const char c, uint_fast8_t& nibble) {
  if (isdigit(c)) {
    nibble = c - 0x30;
    return true;
  } else if (isxdigit(c)) {
    nibble = (c & 0xF) + 9;
    return true;
  } else {
    return false;
  }
}

#define DECODER_ENTRY_CHECKS(state)            \
  MCU_DCHECK_LT(0, state.input_buffer.size()); \
  MCU_DCHECK_LT(state.input_buffer.size(), StringView::kMaxSize)

#define REPORT_ILLFORMED(state, msg_literal) \
  state.OnIllFormed(MCU_PSD(msg_literal))

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
    return state.OnIllFormed(error_message);
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
    state.OnPartialText(token_type, EPartialTokenPosition::kMiddle,
                        partial_token);
  } else {
    // The first character beyond the current segment is in this buffer. It is
    // possible that it is the first character, but we don't have a
    // specialization for that case on the assumption that is relatively rare.
    const auto partial_token = state.input_buffer.prefix(beyond);
    state.input_buffer.remove_prefix(beyond);
    state.SetDecodeFunction(next_decoder);
    state.OnPartialText(token_type, EPartialTokenPosition::kLast,
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
  state.SetDecodeFunction(remainder_decoder);
  state.OnPartialText(token_type, EPartialTokenPosition::kFirst, partial_token);
  return EDecodeBufferStatus::kDecodingInProgress;
}

// An entity (param segment, param name or value) is split, either across input
// buffers or by encoded characters. This function provides decoding of the
// remainder of the entity that is in the input buffer. So far (August 7, 2022)
// this is the only method that contains a loop inside it, besides of course
// DecodeBuffer.
EDecodeBufferStatus PercentEncodedEntityHelper(
    ActiveDecodingState& state, const CharMatchFunction char_matcher,
    const EPartialToken token_type, const DecodeFunction next_decoder) {
  DECODER_ENTRY_CHECKS(state);
  MCU_DCHECK(token_type == EPartialToken::kPathSegment ||
             token_type == EPartialToken::kParamName ||
             token_type == EPartialToken::kParamValue)
      << MCU_NAME_VAL(token_type);  // COV_NF_LINE
  const auto this_decoder = state.GetDecodeFunction();
  do {
    auto beyond = FindFirstNotOf(state.input_buffer, char_matcher);
    if (beyond > 0) {
      // We've got some non-special characters, which we pass to the listener.
      if (beyond == StringView::kMaxSize) {
        beyond = state.input_buffer.size();
      }
      const auto text = state.input_buffer.prefix(beyond);
      state.input_buffer.remove_prefix(beyond);
      state.OnPartialText(token_type, EPartialTokenPosition::kMiddle, text);
      continue;
    }

    const char next = state.input_buffer.front();
    if (next == '%') {
      // The component has a percent-encoded character.
      if (state.input_buffer.size() < 3) {
        return EDecodeBufferStatus::kNeedMoreInput;
      }
      uint_fast8_t high, low;
      if (HexCharToNibble(state.input_buffer.at(1), high) &&
          HexCharToNibble(state.input_buffer.at(2), low)) {
        char c = static_cast<char>(high << 4 | low);
        StringView decoded(&c, 1);
        state.input_buffer.remove_prefix(3);
        state.OnPartialText(token_type, EPartialTokenPosition::kMiddle,
                            decoded);
        continue;
      } else {
        // One of the characters isn't a hexadecimal digit, so it wasn't
        // properly encoded. We treat this as an error.
        return REPORT_ILLFORMED(state, "Invalid Percent-Encoded Character");
      }
    }

    if (next == '+') {
      // The component has a space encoded as a plus. It must not be a path
      // segment, which should be ensured by the char_matcher function.
      MCU_DCHECK_NE(token_type, EPartialToken::kPathSegment);
      state.input_buffer.remove_prefix(1);
      char c = ' ';
      state.OnPartialText(token_type, EPartialTokenPosition::kMiddle,
                          StringView(&c, 1));
      continue;
    }

    // We've reached the end of the split parameter component.
    state.SetDecodeFunction(next_decoder);
    state.OnPartialText(token_type, EPartialTokenPosition::kLast, StringView());
    return EDecodeBufferStatus::kDecodingInProgress;
  } while (this_decoder == state.GetDecodeFunction() &&
           !state.input_buffer.empty());
  return EDecodeBufferStatus::kDecodingInProgress;
}

////////////////////////////////////////////////////////////////////////////////
// Decoder functions for different phases of decoding. Generally in reverse
// order to avoid forward declarations.

// Required forward declarations.
DECODER_FUNCTION(DecodeHeaderLines);
DECODER_FUNCTION(DecodeOptionalParamSeparators);
DECODER_FUNCTION(DecodePathSegment);

EDecodeBufferStatus MatchedEndOfHeaderLine(ActiveDecodingState& state) {
  state.SetDecodeFunction(DecodeHeaderLines);
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
    state.SetDecodeFunction(MatchHeaderValueEnd);
    state.OnCompleteText(EToken::kHeaderValue, value);
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
    state.SetDecodeFunction(DecodeHeaderValue);
  }
  return EDecodeBufferStatus::kDecodingInProgress;
}

DECODER_FUNCTION(MatchHeaderNameValueSeparator) {
  DECODER_ENTRY_CHECKS(state);
  if (state.input_buffer.starts_with(':')) {
    // Found the ':' separating the name from value.
    state.input_buffer.remove_prefix(1);
    state.SetDecodeFunction(SkipOptionalWhitespace);
    return EDecodeBufferStatus::kDecodingInProgress;
  }
  return REPORT_ILLFORMED(state, "Expected colon after name");
}

// The header name is apparently too long to fit in a buffer, so we pass
// whatever we can find in the input_buffer to the listener.
DECODER_FUNCTION(DecodePartialHeaderName) {
  return DecodePartialTokenHelper(state, IsTokenChar,
                                  EPartialToken::kHeaderName,
                                  MatchHeaderNameValueSeparator);
}

DECODER_FUNCTION(DecodePartialHeaderNameStart) {
  return DecodePartialTokenStartHelper(
      state, IsTokenChar, EPartialToken::kHeaderName, DecodePartialHeaderName);
}

EDecodeBufferStatus MatchedEndOfHeaderLines(ActiveDecodingState& state) {
  state.OnHeadersEnd();
  return EDecodeBufferStatus::kComplete;
}

// We're at the start of a header line, or at the end of the headers.
DECODER_FUNCTION(DecodeHeaderLines) {
  DECODER_ENTRY_CHECKS(state);
  const auto beyond = FindFirstNotOf(state.input_buffer, IsTokenChar);
  if (0 < beyond && beyond < StringView::kMaxSize) {
    // We've got a non-empty field name.
    const auto name = state.input_buffer.prefix(beyond);
    state.input_buffer.remove_prefix(beyond);
    state.SetDecodeFunction(MatchHeaderNameValueSeparator);
    state.OnCompleteText(EToken::kHeaderName, name);
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
  state.SetDecodeFunction(DecodeHeaderLines);
  state.OnEvent(EEvent::kHttpVersion1_1);
  return EDecodeBufferStatus::kDecodingInProgress;
}

DECODER_FUNCTION(DecodeHttpVersion) {
  DECODER_ENTRY_CHECKS(state);
  return SkipLiteralAndDispatch(state, MCU_PSV("HTTP/1.1\r\n"),
                                MatchedHttpVersion1_1,
                                MCU_PSD("Unsupported HTTP version"));
}

// We've finished with the path and optional query string, so should be
// looking at a space followed by the HTTP version.
DECODER_FUNCTION(MatchAfterRequestTarget) {
  DECODER_ENTRY_CHECKS(state);
  const char c = state.input_buffer.at(0);
  if (c == ' ') {
    // End of the path, and there is no query.
    state.input_buffer.remove_prefix(1);
    state.SetDecodeFunction(DecodeHttpVersion);
    return EDecodeBufferStatus::kDecodingInProgress;
  }
  MCU_DCHECK(!IsPChar(c)) << c;
  MCU_DCHECK(!IsQueryChar(c)) << c;
  return REPORT_ILLFORMED(state, "Invalid request target end");
}

DECODER_FUNCTION(DecodeRawQueryStringRemainder) {
  return DecodePartialTokenHelper(state, IsQueryChar,
                                  EPartialToken::kRawQueryString,
                                  MatchAfterRequestTarget);
}

DECODER_FUNCTION(DecodeRawQueryStringStart) {
  DECODER_ENTRY_CHECKS(state);
  const char c = state.input_buffer.front();
  if (IsQueryChar(c)) {
    // Starts with a query char. Not optimizing this, just passing through one
    // byte, then delegating to DecodeRawQueryStringRemainder.
    state.input_buffer.remove_prefix(1);
    state.SetDecodeFunction(DecodeRawQueryStringRemainder);
    state.OnPartialText(EPartialToken::kRawQueryString,
                        EPartialTokenPosition::kFirst, StringView(&c, 1));
    return EDecodeBufferStatus::kDecodingInProgress;
  }
  // No query string after the question mark.
  state.SetDecodeFunction(MatchAfterRequestTarget);
  return EDecodeBufferStatus::kDecodingInProgress;
}

// We've reached the end of a parameter value; what's next?
DECODER_FUNCTION(DecodeAfterParamValue) {
  DECODER_ENTRY_CHECKS(state);
  const char next = state.input_buffer.front();
  if (next == '&') {
    state.input_buffer.remove_prefix(1);
    state.SetDecodeFunction(DecodeOptionalParamSeparators);
    return EDecodeBufferStatus::kDecodingInProgress;
  }
  state.SetDecodeFunction(MatchAfterRequestTarget);
  return EDecodeBufferStatus::kDecodingInProgress;
}

DECODER_FUNCTION(DecodeSplitParamValue) {
  return PercentEncodedEntityHelper(state, IsParamValueCharExceptPercentAndPlus,
                                    EPartialToken::kParamValue,
                                    DecodeAfterParamValue);
}

// We're either at the start of a parameter value, or at the end of the query
// string.
DECODER_FUNCTION(DecodeParamValue) {
  DECODER_ENTRY_CHECKS(state);
  const auto beyond =
      FindFirstNotOf(state.input_buffer, IsParamValueCharExceptPercentAndPlus);
  if (beyond == StringView::kMaxSize) {
    // All of the characters are part of a parameter value, but we didn't find
    // the end of the value, so we'll report it to the listener in parts.
    const auto text = state.input_buffer;
    state.input_buffer = StringView();
    state.SetDecodeFunction(DecodeSplitParamValue);
    state.OnPartialText(EPartialToken::kParamValue,
                        EPartialTokenPosition::kFirst, text);
    return EDecodeBufferStatus::kDecodingInProgress;
  }

  const auto text = state.input_buffer.prefix(beyond);
  state.input_buffer.remove_prefix(beyond);
  const char next = state.input_buffer.front();
  if (next == '%' || next == '+') {
    // The value has an encoded/special character, so we're going to need to
    // report it to the listener in parts.
    state.SetDecodeFunction(DecodeSplitParamValue);
    state.OnPartialText(EPartialToken::kParamValue,
                        EPartialTokenPosition::kFirst, text);
    return EDecodeBufferStatus::kDecodingInProgress;
  }

  // `text` has the whole value. What may be next?
  if (state.input_buffer.starts_with('&')) {
    // There may be another parameter.
    state.input_buffer.remove_prefix(1);
    state.SetDecodeFunction(DecodeOptionalParamSeparators);
  } else {
    state.SetDecodeFunction(MatchAfterRequestTarget);
  }
  state.OnCompleteText(EToken::kParamValue, text);
  return EDecodeBufferStatus::kDecodingInProgress;
}

// We've reached the end of a parameter name; what's next?
DECODER_FUNCTION(DecodeAfterParamName) {
  DECODER_ENTRY_CHECKS(state);
  const char next = state.input_buffer.front();
  if (next == '=') {
    state.input_buffer.remove_prefix(1);
    state.SetDecodeFunction(DecodeParamValue);
    return EDecodeBufferStatus::kDecodingInProgress;
  } else if (next == '&') {
    state.input_buffer.remove_prefix(1);
    state.SetDecodeFunction(DecodeOptionalParamSeparators);
    return EDecodeBufferStatus::kDecodingInProgress;
  }
  state.SetDecodeFunction(MatchAfterRequestTarget);
  return EDecodeBufferStatus::kDecodingInProgress;
}

DECODER_FUNCTION(DecodeSplitParamName) {
  return PercentEncodedEntityHelper(state, IsParamNameCharExceptPercentAndPlus,
                                    EPartialToken::kParamName,
                                    DecodeAfterParamName);
}

// We're either at the start of a parameter name, or at the end of the query
// string.
DECODER_FUNCTION(DecodeParamName) {
  DECODER_ENTRY_CHECKS(state);
  const auto beyond =
      FindFirstNotOf(state.input_buffer, IsParamNameCharExceptPercentAndPlus);
  if (beyond == StringView::kMaxSize) {
    // All of the characters are part of a parameter name, but we didn't find
    // the end of the name, so we'll report it to the listener in parts.
    const auto text = state.input_buffer;
    state.input_buffer = StringView();
    state.SetDecodeFunction(DecodeSplitParamName);
    state.OnPartialText(EPartialToken::kParamName,
                        EPartialTokenPosition::kFirst, text);
    return EDecodeBufferStatus::kDecodingInProgress;
  }

  const auto text = state.input_buffer.prefix(beyond);
  state.input_buffer.remove_prefix(beyond);
  const char next = state.input_buffer.front();
  MCU_VLOG_VAR(1, next);
  if (next == '%' || next == '+') {
    // The name has an encoded/special character, so we're going to need to
    // report it to the listener in parts.
    state.SetDecodeFunction(DecodeSplitParamName);
    state.OnPartialText(EPartialToken::kParamName,
                        EPartialTokenPosition::kFirst, text);
    return EDecodeBufferStatus::kDecodingInProgress;
  }

  if (beyond > 0) {
    // `text` has the whole of the parameter name.
    state.SetDecodeFunction(DecodeAfterParamName);
    state.OnCompleteText(EToken::kParamName, text);
    return EDecodeBufferStatus::kDecodingInProgress;
  }

  // We don't have any parameter name characters, so we should be at the end of
  // the request target.
  if (next == '=') {
    return REPORT_ILLFORMED(state, "Param name missing");
  }
  state.SetDecodeFunction(MatchAfterRequestTarget);
  return EDecodeBufferStatus::kDecodingInProgress;
}

// We're either between parameters, in which case we've already located an
// ampersand, or we're at the start of the parameters, in which case we don't
// care if there are leading ampersands or not.
DECODER_FUNCTION(DecodeOptionalParamSeparators) {
  DECODER_ENTRY_CHECKS(state);
  const auto beyond = state.input_buffer.find_first_not_of('&');
  if (0 < beyond) {
    // We found some ampersands.
    if (beyond == StringView::kMaxSize) {
      // We only found ampersands.
      state.input_buffer = StringView();
      return EDecodeBufferStatus::kDecodingInProgress;
    }
    // There is something beyond the ampersands.
    state.input_buffer.remove_prefix(beyond);
  }
  state.SetDecodeFunction(DecodeParamName);
  return EDecodeBufferStatus::kDecodingInProgress;
}

// We've reached the end of valid path characters. Are we at the start of the
// query string, or and the end of the request target?
DECODER_FUNCTION(DecodeEndOfPath) {
  DECODER_ENTRY_CHECKS(state);
  const char c = state.input_buffer.at(0);
  MCU_DCHECK(c != '/' && !IsPChar(c)) << c;
  if (c == '?') {
    // End of the path, start of the query.
    state.input_buffer.remove_prefix(1);
    state.SetDecodeFunction(DecodeOptionalParamSeparators);
    state.OnEvent(EEvent::kPathEndQueryStart);
    return EDecodeBufferStatus::kDecodingInProgress;
  }
  // Appears to be the end of the request target (path and optional query).
  state.SetDecodeFunction(MatchAfterRequestTarget);
  state.OnEvent(EEvent::kPathEnd);
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
    state.SetDecodeFunction(DecodePathSegment);
    state.OnEvent(EEvent::kPathSeparator);
    return EDecodeBufferStatus::kDecodingInProgress;
  }
  state.SetDecodeFunction(DecodeEndOfPath);
  return EDecodeBufferStatus::kDecodingInProgress;
}

// The path segment is either too long to fit in a buffer, is split across
// input buffers or contains a percent-encoded character.
DECODER_FUNCTION(DecodeSplitPathSegment) {
  return PercentEncodedEntityHelper(state, IsPCharExceptPercent,
                                    EPartialToken::kPathSegment,
                                    DecodeAfterSegment);
}

// The previous character matched is a '/'. We're either at the beginning of a
// path segment, or at the end of the path.
DECODER_FUNCTION(DecodePathSegment) {
  DECODER_ENTRY_CHECKS(state);
  const auto beyond = FindFirstNotOf(state.input_buffer, IsPCharExceptPercent);
  if (beyond < StringView::kMaxSize) {
    // We've found a non-path character or a '%'.
    const auto segment = state.input_buffer.prefix(beyond);
    state.input_buffer.remove_prefix(beyond);
    const bool has_percent = state.input_buffer.starts_with('%');
    if (0 < beyond && !has_percent) {
      // We've got the entire, non-empty segment, and it doesn't have any
      // percent-encoded characters in it, which makes it easier to decode.
      state.SetDecodeFunction(DecodeAfterSegment);
      state.OnCompleteText(EToken::kPathSegment, segment);
      return EDecodeBufferStatus::kDecodingInProgress;
    } else if (has_percent) {
      // There are percent-encoded characters, so we need to do extra work. We
      // handle this by using separate decoder functions for the encoded vs.
      // the "normal" characters.
      state.SetDecodeFunction(DecodeSplitPathSegment);
      state.OnPartialText(EPartialToken::kPathSegment,
                          EPartialTokenPosition::kFirst, segment);
      return EDecodeBufferStatus::kDecodingInProgress;
    } else {
      // The next character isn't a path character, so should be at the end of
      // the path.
      MCU_DCHECK_EQ(segment.size(), 0);
      state.SetDecodeFunction(DecodeEndOfPath);
      return EDecodeBufferStatus::kDecodingInProgress;
    }
  } else {
    // The input_buffer isn't empty, and it consists only of path characters,
    // so we can't tell where the end of the segment will be. This may
    // indicate that the path segment is longer than the maximum size of the
    // caller's buffer. Since someone will need to do the buffering of the
    // portion of the input already available, and in the case of the caller,
    // it may need to copy the input from the end of its buffer to the start
    // in order to make room for more data, we choose instead to provide the
    // available data to the listener, which will allow us to empty the input
    // buffer. Sometimes this may be sub-optimal, but it will always eliminate
    // another scan of the leading portion of the path segment.
    const auto segment = state.input_buffer;
    state.input_buffer = StringView();
    state.SetDecodeFunction(DecodeSplitPathSegment);
    state.OnPartialText(EPartialToken::kPathSegment,
                        EPartialTokenPosition::kFirst, segment);
    return EDecodeBufferStatus::kDecodingInProgress;
  }
}

DECODER_FUNCTION(DecodeStartOfPath) {
  DECODER_ENTRY_CHECKS(state);
  if (state.input_buffer.starts_with('/')) {
    // We're decoding an origin-form request-target.
    // https://datatracker.ietf.org/doc/html/rfc7230#section-5.3.1
    state.input_buffer.remove_prefix(1);
    state.SetDecodeFunction(DecodePathSegment);
    state.OnEvent(EEvent::kPathStart);
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

  const auto beyond = FindFirstNotOf(state.input_buffer, IsMethodChar);
  if (0 < beyond && beyond < StringView::kMaxSize) {
    // We've got a non-method char after a method char (or several).
    const auto method = state.input_buffer.prefix(beyond);
    state.input_buffer.remove_prefix(beyond);
    if (!state.input_buffer.starts_with(' ')) {
      return REPORT_ILLFORMED(state, "Invalid HTTP method end");
    }
    state.input_buffer.remove_prefix(1);
    state.SetDecodeFunction(DecodeStartOfPath);
    state.OnCompleteText(EToken::kHttpMethod, method);
    return EDecodeBufferStatus::kDecodingInProgress;
  } else if (beyond == 0) {
    return REPORT_ILLFORMED(state, "Invalid HTTP method start");
  } else {
    // Haven't found the end of the method. Caller will need to figure out
    // if more input is possible.
    state.partial_decode_function_if_full = DecodeHttpMethodError;
    return EDecodeBufferStatus::kNeedMoreInput;
  }
}

DECODER_FUNCTION(DecodeInternalError) {
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
  /*****************************************************************************
Command for generating the 'table' below:

egrep "^DECODER_FUNCTION\(\w+\) \{" mcunet/src/http1/request_decoder.cc | \
  cut '-d ' -f1 | \
  sed -e "s/DECODER_FUNCTION/  OUTPUT_METHOD_NAME/ ; s/$/;/" | \
  (echo;sort;echo)

*******************************************************************************/

  OUTPUT_METHOD_NAME(DecodeAfterParamName);
  OUTPUT_METHOD_NAME(DecodeAfterParamValue);
  OUTPUT_METHOD_NAME(DecodeAfterSegment);
  OUTPUT_METHOD_NAME(DecodeEndOfPath);
  OUTPUT_METHOD_NAME(DecodeHeaderLines);
  OUTPUT_METHOD_NAME(DecodeHeaderValue);
  OUTPUT_METHOD_NAME(DecodeHttpMethod);
  OUTPUT_METHOD_NAME(DecodeHttpMethodError);
  OUTPUT_METHOD_NAME(DecodeHttpVersion);
  OUTPUT_METHOD_NAME(DecodeInternalError);
  OUTPUT_METHOD_NAME(DecodeOptionalParamSeparators);
  OUTPUT_METHOD_NAME(DecodeParamName);
  OUTPUT_METHOD_NAME(DecodeParamValue);
  OUTPUT_METHOD_NAME(DecodePartialHeaderName);
  OUTPUT_METHOD_NAME(DecodePartialHeaderNameStart);
  OUTPUT_METHOD_NAME(DecodePartialHeaderValue);
  OUTPUT_METHOD_NAME(DecodePartialHeaderValueStart);
  OUTPUT_METHOD_NAME(DecodePathSegment);
  OUTPUT_METHOD_NAME(DecodeRawQueryStringRemainder);
  OUTPUT_METHOD_NAME(DecodeRawQueryStringStart);
  OUTPUT_METHOD_NAME(DecodeSplitParamName);
  OUTPUT_METHOD_NAME(DecodeSplitParamValue);
  OUTPUT_METHOD_NAME(DecodeSplitPathSegment);
  OUTPUT_METHOD_NAME(DecodeStartOfPath);
  OUTPUT_METHOD_NAME(MatchAfterRequestTarget);
  OUTPUT_METHOD_NAME(MatchHeaderNameValueSeparator);
  OUTPUT_METHOD_NAME(MatchHeaderValueEnd);
  OUTPUT_METHOD_NAME(SkipOptionalWhitespace);

#undef OUTPUT_METHOD_NAME

  MCU_CHECK(false) << MCU_PSD(                            // COV_NF_LINE
      "Haven't implemented a case for decode_function");  // COV_NF_LINE
  return 0;                                               // COV_NF_LINE
}

RequestDecoderImpl::RequestDecoderImpl() {
  decode_function_ = DecodeInternalError;
}

void RequestDecoderImpl::Reset() { decode_function_ = DecodeHttpMethod; }

EDecodeBufferStatus RequestDecoderImpl::DecodeBuffer(
    StringView& buffer, RequestDecoderListener& listener,
    const bool buffer_is_full) {
  MCU_VLOG(1) << MCU_PSD("ENTER DecodeBuffer size=") << buffer.size()
              << MCU_NAME_VAL(buffer_is_full);

  MCU_DCHECK_LT(0, buffer.size());
  MCU_DCHECK_LT(buffer.size(), StringView::kMaxSize);

  MCU_CHECK_NE(decode_function_, nullptr);

  const auto start_size = buffer.size();

  ActiveDecodingState active_state(buffer, listener, *this);
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
    MCU_VLOG(2) << old_decode_function << MCU_PSD(" returned")
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
        // full as it can get, so we either need a fallback decoder, or we
        // have to report an error.
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

void BaseListenerCallbackData::SkipQueryStringDecoding() const {
  MCU_DCHECK_EQ(state.GetDecodeFunction(), DecodeOptionalParamSeparators);
  state.SetDecodeFunction(DecodeRawQueryStringStart);
}

}  // namespace http1
}  // namespace mcunet
