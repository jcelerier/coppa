#pragma once
#include <oscpack/ip/UdpSocket.h>
#include <oscpack/osc/OscOutboundPacketStream.h>
#include <coppa/protocol/osc/oscmessagegenerator.hpp>

#include <string>
#include <iostream>
#include <memory>
namespace coppa
{
namespace osc
{
class sender
{
  public:
    sender() = default;
    sender& operator=(const sender&) = default;
    sender(const std::string& ip, const int port):
      m_socket{std::make_shared<UdpTransmitSocket>(IpEndpointName(ip.c_str(), port))},
      m_ip(ip),
      m_port(port)
    {
    }

    virtual ~sender() = default;
    sender(sender&&) = default;
    sender(const sender&) = delete;

    template<typename... Args>
    void send(std::string address, Args&&... args)
    {
      send(::oscpack::MessageGenerator<>()(address, std::forward<Args>(args)...));
    }

    const std::string& ip() const { return m_ip; }
    int port() const { return m_port; }

  private:
    void send(const ::oscpack::OutboundPacketStream& m)
    {
      m_socket->Send( m.Data(), m.Size() );
    }

    std::shared_ptr<UdpTransmitSocket> m_socket;
    std::string m_ip;
    int m_port;
};

}
}
