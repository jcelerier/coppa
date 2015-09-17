#pragma once
#include <coppa/coppa.hpp>
#include <KF5/KDNSSD/DNSSD/PublicService>
#include <string>
#include <thread>
#include <QDebug>

namespace coppa
{
/**
 * @brief The zeroconf_server class
 *
 * A server mixin that adds zeroconf capabilities.
 */
template<typename Server>
class zeroconf_server : private Server
{
    const int m_port = 9002;
    std::unique_ptr<KDNSSD::PublicService> m_zeroconf_service;

  public:
    using Server::setOpenHandler;
    using Server::setCloseHandler;
    using Server::setMessageHandler;
    using Server::sendMessage;

    zeroconf_server() = default;
    zeroconf_server(zeroconf_server&& other):
      m_port{other.m_port}
    {
      m_zeroconf_service = std::move(other.m_zeroconf_service);
    }

    template<typename... Args>
    zeroconf_server(std::string text, int port, Args&&... args):
      Server{std::forward<Args>(args)...},
      m_port{port},
      m_zeroconf_service{std::make_unique<KDNSSD::PublicService>(
                           text.c_str(),
                           "_coppa._tcp",
                           port)}
    {
    }

    using connection_handler = typename Server::connection_handler;

    void run()
    {
      if(m_zeroconf_service)
        m_zeroconf_service->publishAsync();
      Server::run(m_port);
    }
};
}
