#include "extras/host/ethernet5500/ethernet_server.h"

#include <ios>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "extras/host/ethernet5500/ethernet_class.h"
#include "extras/host/ethernet5500/ethernet_config.h"
#include "extras/host/ethernet5500/w5500.h"  // IWYU pragma: export
#include "platform_network_interface.h"

EthernetServer::EthernetServer(uint16_t port) : port_(port) {
  CHECK_GT(port_, 0);
}

void EthernetServer::begin() {
  VLOG(3) << "EthernetServer::begin entry, port_ is " << port_;
  auto* const platform_network =
      mcunet::PlatformNetworkInterface::GetImplementationOrDie();
  auto sock_num = platform_network->FindUnusedSocket();
  if (sock_num >= 0) {
    VLOG(3) << "EthernetClass::_server_port[" << sock_num
            << "] = " << EthernetClass::_server_port[sock_num];
    if (platform_network->InitializeTcpListenerSocket(sock_num, port_)) {
      EthernetClass::_server_port[sock_num] = port_;
    } else {
      LOG(ERROR) << "Unable to start listener for TCP port " << port_;
    }
    VLOG(3) << "EthernetClass::_server_port[" << sock_num
            << "] = " << EthernetClass::_server_port[sock_num];
  } else {
    LOG(ERROR) << "Unable to find a socket available for listening to port "
               << port_;
  }
  VLOG(3) << "EthernetServer::begin exit";
}

void EthernetServer::accept() {
  VLOG(3) << "EthernetServer::accept() entry, port_ is " << port_;
  auto* const platform_network =
      mcunet::PlatformNetworkInterface::GetImplementationOrDie();
  // Not the same as Berkeley Socket API's ::accept(). It basically takes care
  // of accepting new connections for the specified port, if there are any,
  // and it shuts down sockets where we've reached EOF reading from the client;
  // this isn't a great behavior because it doesn't allow for the EOF to be a
  // signal from the client that all the data has been sent, and the server
  // should now send its response.
  int listening = 0;
  for (int sock_num = 0; sock_num < MAX_SOCK_NUM; sock_num++) {
    VLOG(3) << "EthernetClass::_server_port[" << sock_num
            << "] = " << EthernetClass::_server_port[sock_num];
    if (EthernetClass::_server_port[sock_num] == port_) {
      EthernetClient client(sock_num);
      auto status = client.status();
      VLOG(3) << "EthernetServer::accept found socket " << sock_num
              << " listening to port " << port_ << " with status " << std::hex
              << (status + 0);
      if (status == SnSR::LISTEN) {
        listening = 1;
        if (platform_network->AcceptConnection(sock_num)) {
          VLOG(3) << "Socket " << sock_num << " has received a connection.";
        } else {
          VLOG(3) << "Socket " << sock_num << " is listening.";
        }
      } else if (status == SnSR::CLOSE_WAIT && !client.available()) {
        VLOG(3) << "Socket " << sock_num
                << " half closed and there is no more data available.";
        client.stop();
      }
    }
  }
  if (!listening) {
    VLOG(3) << "EthernetServer::accept found no socket listening to port "
            << port_
            << "; will call EthernetServer::begin to find a free socket.";
    begin();
  }
  VLOG(3) << "EthernetServer::accept() exit";
}

EthernetClient EthernetServer::available() {
  VLOG(3) << "EthernetServer::available() entry, port_ is " << port_;
  accept();
  VLOG(3) << "EthernetServer::available() loop entry";
  for (int sock_num = 0; sock_num < MAX_SOCK_NUM; sock_num++) {
    VLOG(3) << "EthernetClass::_server_port[" << sock_num
            << "] = " << EthernetClass::_server_port[sock_num];
    EthernetClient client(sock_num);
    if (EthernetClass::_server_port[sock_num] == port_ &&
        (client.status() == SnSR::ESTABLISHED ||
         client.status() == SnSR::CLOSE_WAIT)) {
      VLOG(3) << "available: Socket " << sock_num
              << " is serving a connection to port " << port_;
      if (client.available()) {
        // XXX: don't always pick the lowest numbered socket.
        return client;
      }
    }
  }
  return EthernetClient(MAX_SOCK_NUM);
}
