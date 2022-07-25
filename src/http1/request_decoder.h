#ifndef MCUNET_SRC_HTTP1_REQUEST_DECODER_H_
#define MCUNET_SRC_HTTP1_REQUEST_DECODER_H_

// mcunet::http1::RequestDecoder is an HTTP/1.1 Request Message (but not body)
// decoder for tiny web servers, i.e. those intended to run on a very low-memory
// system, such as on a microcontroller. The aim is to be able to decode most
// HTTP/1.1 requests, though with the limitation that it can't interpret
// 'tokens' that are longer than the maximum size buffer that the system can
// support (i.e. if the name of a header is longer than that limit, the listener
// will be informed of the issue, and the header name and value will be skipped
// over).
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
// NOTE: While I was tempted to avoid using virtual functions in the listener
// API in order to avoid virtual-function-tables, which are stored in RAM when
// compiled by avr-gcc, I decided that doing so was likely to increase the
// complexity of this class (e.g. multiple listener methods, and multiple
// context pointers) and thus of the application using this class. Until I have
// evidence otherwise, I'll use a single listener, an instance of a class that
// requires virtual functions.

#include <McuCore.h>

#include "http1/request_decoder_constants.h"

namespace mcunet {
// I don't normally want to have many nested namespaces, but the decoder has a
// helper class and several enum definitions, with names that I'd like to keep
// short, and yet not readily conflict with other similar enums, so I've chosen
// to put them into a nested namespace.
namespace http1 {

// Note that percent encoding is not been removed from text before it is passed
// to the listener.
struct RequestDecoderListener {
  virtual ~RequestDecoderListener() {}
  virtual void OnEvent(EEvent event) = 0;

  // Notifies the listener that the complete value of the specified token is in
  // `text`.
  virtual void OnCompleteText(EToken token, mcucore::StringView text) = 0;

  // TODO(jamessynge): Consider whether to add arg(s) to OnPartialText to
  // indicate if it is the first call or the last call of the sequence of calls
  // for an oversize token value (e.g. an enum EPartialTokenPosition {kFirst,
  // kMiddle, kEnd};). That obviates the need to add an OnStartPartialText,
  // and *may* simplify listener impls.

  // Notifies the listener that *some* of the value of the specified token is in
  // `text`. This should only happen if the entity being decoded it longer than
  // the maximum size that the caller can provide in a single StringView.
  virtual void OnPartialText(EPartialToken token,
                             EPartialTokenPosition position,
                             mcucore::StringView text) = 0;

  // All done.
  virtual void OnEnd() = 0;

  // TODO(jamessynge): Consider changing msg type to ArrayView<ProgmemString> or
  // ArrayView<AnyPrintable>. This assumes that the listener has the ability to
  // emit or store the error message based on the msg.
  virtual void OnError(mcucore::ProgmemString msg) = 0;
};

struct ActiveDecodingState;

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
  void SetListener(RequestDecoderListener& listener);
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

  // Event delivery methods. These allow us to log centrally, and to deal with
  // an unset listener centrally.
  void OnEvent(EEvent event);
  void OnCompleteText(EToken token, mcucore::StringView text);
  void OnPartialText(EPartialToken token, EPartialTokenPosition position,
                     mcucore::StringView text);
  void OnEnd();
  void OnError(mcucore::ProgmemString msg);
  EDecodeBufferStatus OnIllFormed(mcucore::ProgmemString msg);
  void SetDecodeFunction(DecodeFunction decode_function);

 private:
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

}  // namespace http1
}  // namespace mcunet

#endif  // MCUNET_SRC_HTTP1_REQUEST_DECODER_H_
