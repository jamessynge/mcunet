#ifndef MCUNET_SRC_HTTP1_REQUEST_DECODER_H_
#define MCUNET_SRC_HTTP1_REQUEST_DECODER_H_

// mcunet::http1::RequestDecoder is an HTTP/1.1 Request Message (but not body)
// decoder for tiny web servers, i.e. those intended to run on a very low-memory
// system, such as on a microcontroller. HTTP/1.1 is specified in RFC 9112
// (June, 2022), which obsoletes RFC 7230 (June, 2014).
//
//    https://www.rfc-editor.org/rfc/rfc9112.html
//
// The aim is to be able to decode most HTTP/1.1 requests, though with the
// limitation that it can't interpret 'tokens' that are longer than the maximum
// size buffer that the system can support (i.e. if the name of a header is
// longer than that limit, the listener will be informed of the issue, and the
// header name and value will be skipped over).
//
// This originated as RequestDecoder in TinyAlpacaServer, which was targeted at
// a very specific set of URL paths and header names.
//
// Because there is no standard format for the query string, processing it is
// delegated to the listener. However, given application/x-www-form-urlencoded
// is the most common format in which query strings are encoded, and is the
// manner in which form data is submitted from a browser (and is the encoded
// specified by the ASCOM Alpaca standard), a decoder is provided by
// WwwFormUrlEncodedDecoder.
//
// Similarly, there isn't a single format for header values, so parsing them is
// delegated to the listener.
//
// NOTE: While I was tempted to avoid using virtual functions in the listener
// API in order to avoid virtual-function-tables, which are stored in RAM when
// compiled by avr-gcc, I decided that doing so was likely to increase the
// complexity of this class (e.g. multiple listener methods, and perhaps
// multiple context pointers) and thus of the application using this class.
// Until I have evidence otherwise, I'll use a single listener, an instance of a
// class that requires virtual functions.

#include <McuCore.h>

#include "http1/request_decoder_constants.h"

// I don't normally want to have many nested namespaces, but the decoder has
// helper classes and several enum definitions, with names that I'd like to keep
// relatively short, and yet not readily conflict with other similar enums, so
// I've chosen to use nested namespaces for the decoder and helpers.
namespace mcunet {
namespace http1 {

// Forward declarations.
struct ActiveDecodingState;
struct RequestDecoderListener;

// The decoder will use an instance of one of the classes derived from
// BaseListenerCallbackData to provide data to the listener at key points in the
// decoding process.
struct BaseListenerCallbackData {
  explicit BaseListenerCallbackData(const ActiveDecodingState& state)
      : state(state) {}

  // Change the listener.
  void SetListener(RequestDecoderListener* listener) const;

  // Puts the decoder into an error state, with the effect that it stops
  // decoding and returns kInternalError.
  void StopDecoding() const;

  // Returns a view of the data passed into RequestDecoder::DecodeBuffer.
  mcucore::StringView GetFullDecoderInput() const;

 private:
  const ActiveDecodingState& state;
};

struct OnEventData : BaseListenerCallbackData {
  EEvent event;
};

struct OnPartialTextData : BaseListenerCallbackData {
  EPartialToken token;
  EPartialTokenPosition position;
  mcucore::StringView text;
};

struct OnCompleteTextData : BaseListenerCallbackData {
  // This ctor exists only to help with implementing tests, in particular for
  // CollapsingRequestDecoderListener, which creates one OnCompleteTextData
  // instance for each string of related OnPartialText calls.
  explicit OnCompleteTextData(const BaseListenerCallbackData& partial_data)
      : BaseListenerCallbackData(partial_data) {}
  EToken token;
  mcucore::StringView text;
};

struct OnErrorData : BaseListenerCallbackData {
  mcucore::ProgmemString message;
  mcucore::StringView undecoded_input;
};

// Note that percent encoding is not been removed from text before it is passed
// to the listener.
struct RequestDecoderListener {
  virtual ~RequestDecoderListener() {}

  // `data.event` has occurred; usually means that some particular fixed text
  // has been matched.
  virtual void OnEvent(const OnEventData& data) = 0;

  // `data.token` has been matched; it's complete value is in `data.text`.
  virtual void OnCompleteText(const OnCompleteTextData& data) = 0;

  // Some portion of `data.token` has been matched, with that portion in
  // `data.text`. The text may be empty, so far just for positions kFirst and
  // kLast.
  virtual void OnPartialText(const OnPartialTextData& data) = 0;

  // An error has occurred, as described in `data.message`.
  virtual void OnError(const OnErrorData& data) = 0;
};

class RequestDecoderImpl {
 public:
  using DecodeFunction = EDecodeBufferStatus (*)(ActiveDecodingState& state);

  RequestDecoderImpl();

  // Reset the decoder, ready to decode a new request. Among other things, this
  // clears the listener, so SetListener must be called if decoding events are
  // desired (most likely!). The reason to clear the listener is the expectation
  // that different listeners will be used for different situations (i.e. you
  // might switch listeners to choose a listener appropriate for a particular
  // path prefix).
  void Reset();

  // Set the listener to be used when DecodeBuffer is next called.
  void SetListener(RequestDecoderListener& listener);

  // Clear listener_ (set to nullptr). This means that the decoder will drop
  // events on the floor. This could be useful if one wants to discard the
  // remainder of a message header, continuing decoding until EEvent::kComplete
  // is returned by DecodeBuffer.
  void ClearListener();

  // Decodes some or all of the contents of buffer. If buffer_is_full is true,
  // then it is assumed that size represents the maximum amount of data that can
  // be provided at once. If an element of the HTTP request, e.g. a header name,
  // that we're decoding is determined to be longer than can fit in a full
  // buffer, then it is passed to the listener via the OnPartialText method,
  // over as many DecodeBuffer calls as required to reach the end of that
  // element of the HTTP request; otherwise OnCompleteText is used.
  EDecodeBufferStatus DecodeBuffer(mcucore::StringView& buffer,
                                   bool buffer_is_full);

 private:
  friend class ActiveDecodingState;

  // Pointer to the next function to be used for decoding. Changes as different
  // entities in a request header is matched.
  DecodeFunction decode_function_;

  // If nullptr, no events are delivered by the decoder, but decoding continues.
  // This provides a means to deal with ill-formed or unsupported requests.
  RequestDecoderListener* listener_{nullptr};
};

// Decodes HTTP/1.1 request headers.
class RequestDecoder : /*private*/ RequestDecoderImpl {
 public:
  using RequestDecoderImpl::RequestDecoderImpl;

  using RequestDecoderImpl::ClearListener;
  using RequestDecoderImpl::DecodeBuffer;
  using RequestDecoderImpl::Reset;
  using RequestDecoderImpl::SetListener;
};

// Placing these functions into namespace mcunet_http1_internal so they can be
// called by unit tests. They are not a part of the public API of the decoder.
namespace mcunet_http1_internal {

// Match characters allowed BY THIS DECODER in a method name.
bool IsMethodChar(const char c);

// Match characters allowed in a path segment.
bool IsPChar(const char c);

// Match characters allowed in a query string.
bool IsQueryChar(const char c);

// Match characters allowed in a token (e.g. a header name).
bool IsTokenChar(const char c);

// Match characters allowed in a header value, per RFC7230, Section 3.2.6.
bool IsFieldContent(const char c);

// Return the index of the first character that doesn't match the test function.
// Returns StringView::kMaxSize if not found.
mcucore::StringView::size_type FindFirstNotOf(const mcucore::StringView& view,
                                              bool (*test)(char));

// Removes leading whitespace characters, returns true when the first character
// is not whitespace.
bool SkipLeadingOptionalWhitespace(mcucore::StringView& view);

// Remove optional whitespace from the end of the view, which is non-empty.
// Returns true if optional whitespace was removed.
bool TrimTrailingOptionalWhitespace(mcucore::StringView& view);

}  // namespace mcunet_http1_internal
}  // namespace http1
}  // namespace mcunet

#endif  // MCUNET_SRC_HTTP1_REQUEST_DECODER_H_
