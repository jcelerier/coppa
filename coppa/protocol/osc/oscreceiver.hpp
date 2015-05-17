#pragma once

#include <oscpack/ip/UdpSocket.h>
#include <oscpack/osc/OscPacketListener.h>
#include <memory>
#include <thread>
#include <functional>
#include <map>
#include <iostream>

class OscReceiver
{
  public:
    using message_handler = std::function<void(const osc::ReceivedMessage&)>;
    //using connection_handler = std::function<void(osc::ReceivedMessageArgumentStream, std::string)>;

    OscReceiver(unsigned int port, message_handler&& msg):
      m_impl{std::move(msg)}
    {
      setPort(port);
    }

    ~OscReceiver()
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
/*
    template<typename T, class C>
    void addHandler(const std::string &s, T&& theMember, C&& theObject)
    {
      m_impl.addHandler(s, std::bind(theMember, theObject, std::placeholders::_1));
    }

    void addHandler(const std::string &s, const	message_handler h)
    {
      m_impl.addHandler(s, h);
    }
*/
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

//      std::cerr << "Receiver port set : " << _port << std::endl;
      return m_port;
    }

  private:
    unsigned int m_port = 0;
    std::shared_ptr<UdpListeningReceiveSocket> m_socket;
    class Impl: public osc::OscPacketListener
    {
      public:
        Impl(message_handler&& msg):
          m_messageHandler{std::move(msg)}
        {
        }
        /*
        void setConnectionHandler(const std::string& s, const connection_handler& h)
        {
          _connectionAdress = s;
          _connectionHandler = h;
        }

        void setMessageHandler(const std::string& s, const message_handler& h)
        {
          _connectionAdress = s;
          _connectionHandler = h;
        }
        */

      protected:
        virtual void ProcessMessage(const osc::ReceivedMessage& m,
                                    const IpEndpointName& ip) override
        {

          try
          {
            m_messageHandler(m);
            /*
            auto addr = std::string(m.AddressPattern());
            std::cerr << "Message received on " << addr << std::endl;
            if(addr == _connectionAdress)
            {
              char s[16];
              ip.AddressAsString(s);
              _connectionHandler(m.ArgumentStream(), std::string(s));
            }
            else if(_map.find(addr) != _map.end())
            {
              _map[addr](m.ArgumentStream());
            }
            */
          }
          catch( osc::Exception& e )
          {
            std::cerr << "OSC Parse Error on " << m.AddressPattern() << ": "
                      << e.what() << std::endl;
          }

        }

      private:
        //std::string _connectionAdress;
        //connection_handler _connectionHandler;
        message_handler m_messageHandler;
    } m_impl;

    std::thread m_runThread;
};
