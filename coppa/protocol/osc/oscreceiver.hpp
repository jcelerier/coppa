#pragma once

#include <oscpack/ip/UdpSocket.h>
#include <oscpack/osc/OscPacketListener.h>
#include <memory>
#include <thread>
#include <functional>
#include <iostream>

namespace coppa
{
namespace osc
{
class receiver
{
  public:
    using message_handler = std::function<void(const ::oscpack::ReceivedMessage&)>;

    receiver(unsigned int port, message_handler&& msg):
      m_impl{std::move(msg)}
    {
      setPort(port);
    }

    ~receiver()
    {
      m_socket->AsynchronousBreak();
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      if(m_runThread.joinable())
        m_runThread.detach();
      m_socket.reset();
    }

    void run()
    {
      m_runThread = std::thread(&UdpListeningReceiveSocket::Run, m_socket.get());
    }

    unsigned int port() const
    {
      return m_port;
    }

    unsigned int setPort(unsigned int port)
    {
      m_port = port;

      bool ok = false;
      while(!ok)
      {
        try
        {
          m_socket = std::make_shared<UdpListeningReceiveSocket>
                   (IpEndpointName(IpEndpointName::ANY_ADDRESS, m_port),
                    &m_impl);
          ok = true;
        }
        catch(std::runtime_error& e)
        {
          m_port++;
        }
      }

      return m_port;
    }

  private:
    unsigned int m_port = 0;
    std::shared_ptr<UdpListeningReceiveSocket> m_socket;
    class Impl: public ::oscpack::OscPacketListener
    {
      public:
        Impl(message_handler&& msg):
          m_messageHandler{std::move(msg)}
        {
        }

      protected:
        virtual void ProcessMessage(const ::oscpack::ReceivedMessage& m,
                                    const IpEndpointName& ip) override
        {
          try
          {
            m_messageHandler(m);
          }
          catch( std::exception& e )
          {
            std::cerr << "OSC Parse Error on " << m.AddressPattern() << ": "
                      << e.what() << std::endl;
          }
        }

      private:
        message_handler m_messageHandler;
    } m_impl;

    std::thread m_runThread;
};
}
}
