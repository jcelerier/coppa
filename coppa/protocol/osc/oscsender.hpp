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
/**
 * @brief The sender class
 *
 * Sends OSC packets to a given address on an UDP port.
 *
 */
class sender
{
  public:
    sender() = default;
    sender(sender&&) = default;
    sender(const sender&) = delete;
    sender& operator=(const sender&) = default;
    sender& operator=(sender&&) = default;

    sender(const std::string& ip, const int port):
      m_socket{std::make_unique<oscpack::UdpTransmitSocket>(oscpack::IpEndpointName(ip.c_str(), port))},
      m_ip(ip),
      m_port(port)
    {
    }

    template<typename... Args>
    void send(const std::string& address, Args&&... args)
    {
      send_impl(m_gen(
                  address,
                  std::forward<Args>(args)...));
    }


    template<typename... Args>
    void send(string_view address, Args&&... args)
    {
      send_impl(m_gen(
                  address,
                  std::forward<Args>(args)...));
    }


    template<int N, typename... Args>
    void send(oscpack::small_string_base<N> address, Args&&... args)
    {
      send_impl(m_gen(
                  address,
                  std::forward<Args>(args)...));
    }


    const std::string& ip() const { return m_ip; }
    int port() const { return m_port; }

  private:
    void send_impl(const oscpack::OutboundPacketStream& m)
    {
      m_socket->Send( m.Data(), m.Size() );
    }

    std::unique_ptr<oscpack::UdpTransmitSocket> m_socket;
    std::string m_ip;
    int m_port;

    oscpack::MessageGenerator<> m_gen;
};

}
}
