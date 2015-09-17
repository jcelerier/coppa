#pragma once
#include <coppa/coppa.hpp>
#include <KF5/KDNSSD/DNSSD/PublicService>
#include <string>
#include <thread>
#include <QDebug>

namespace coppa
{
// TODO fix naming convention
template<typename Server>
class BonjourServer
{
    Server m_server;
    const int m_port = 9002;
    std::unique_ptr<KDNSSD::PublicService> m_zeroconf_service;

  public:
    BonjourServer() = default;
    BonjourServer(BonjourServer&& other):
      m_port{other.m_port}
    {
      m_zeroconf_service = std::move(other.m_zeroconf_service);
    }

    template<typename... Args>
    BonjourServer(std::string text, int port, Args&&... args):
      m_server{std::forward<Args>(args)...},
      m_port{port},
      m_zeroconf_service{std::make_unique<KDNSSD::PublicService>(
            text.c_str(),
            "_coppa._tcp",
            9002)}
    {
    }

    using connection_handler = typename Server::connection_handler;

    void run()
    {
      if(m_zeroconf_service)
        m_zeroconf_service->publishAsync();
      m_server.run(m_port);
    }

    FORWARD_FUN(m_server, auto, setOpenHandler)
    FORWARD_FUN(m_server, auto, setCloseHandler)
    FORWARD_FUN(m_server, auto, setMessageHandler)
    FORWARD_FUN(m_server, auto, sendMessage)
};
}
