# About McuNet

C++ library providing networking support for Arduino sketches, focused on the
WIZnet W5500 Internet Offload Chip, which is used in the Robotdyn Mega Eth.

This library depends on [McuCore](https://github.com/jamessynge/mcucore).

## Planning and Goals

I want to generalize this so that a sketch or library making use of McuNet
doesn't need to depend on Ethernet5500 Arduino Library (or similar). Instead, it
can just make use of this library as a Hardware Abstraction Layer (HAL).

As part of developing TinyAlpacaServer I wrote an HTTP server specialized for
the ASCOM Alpaca protocol. I wish to generalize that as a TCP and UDP server,
and an HTTP server on top of the TCP server. If the networking HAL is
sufficiently general, it might be best to use separate libraries for the HAL,
for the WIZnet W5500 device driver, and for the servers. I don't know if there
is sufficient justification (other than fun) for doing this. There are plenty of
other developers out there who've done similar work, which it is worth exploring
in detail before making a decision about this.

### TODOs

*  For demos, it is beneficial to NOT wait for DHCP to complete before being
   able to answer Alpaca Discovery requests. So, consider supporting async DHCP
   address allocation requests.

## About extras/host/...

I find that I can develop software (think, code, compile/link, test/debug,
think) much faster by iterating on a host (Linux box, in my case), rather than
re-uploading to the target Arduino for each test/debug phase. Therefore I've
recreated (including copying of APIs) just enough of the Arduino classes on
which this library depends. These are in the folder extras/host in either this
library or McuCore.

I've written most of my tests based on googletest, a C++ Unit Testing framework
from Google. I use it at work and really appreciate what it provides.

*   https://github.com/google/googletest/

### Other On-Host Testing Solutions for Arduino

Others have explored the topic of host based unit testing of Arduino libraries,
and also supporting embedded unit testing of Arduino code:

*   https://github.com/bxparks/EpoxyDuino
*   https://github.com/mmurdoch/arduinounit
*   https://github.com/bxparks/AUnit
