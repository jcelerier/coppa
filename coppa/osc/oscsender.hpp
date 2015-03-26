#pragma once
#include <oscpack/ip/UdpSocket.h>
#include <oscpack/osc/OscOutboundPacketStream.h>
#include <coppa/osc/oscmessagegenerator.hpp>

#include <string>
#include <iostream>
#include <memory>

class OscSender
{
    public:
        OscSender() = default;
        OscSender& operator=(const OscSender&) = default;
        OscSender(const std::string& ip, const int port):
            m_socket{std::make_shared<UdpTransmitSocket>(IpEndpointName(ip.c_str(), port))},
            m_ip(ip),
            m_port(port)
        {
        }

        virtual ~OscSender() = default;
        OscSender(OscSender&&) = default;
        OscSender(const OscSender&) = delete;

        void send(const osc::OutboundPacketStream& m)
        {
            m_socket->Send( m.Data(), m.Size() );
        }

        template<typename... Args>
        void send(std::string address, Args&&... args)
        {
            send(osc::MessageGenerator()(address, std::forward<Args>(args)...));
        }

        const std::string& ip() const { return m_ip; }
        int port() const { return m_port; }

    private:
        std::shared_ptr<UdpTransmitSocket> m_socket;
        std::string m_ip;
        int m_port;
};
