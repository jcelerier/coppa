#pragma once
#define ASIO_STANDALONE 1
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/http/request.hpp>

#include <coppa/exceptions/BadRequest.hpp>
namespace coppa
{
namespace ws
{
class server
{
  public:
    using server_t = websocketpp::server<websocketpp::config::asio>;
    using connection_handler = websocketpp::connection_hdl;

    server()
    {
      m_server.init_asio();
      m_server.set_reuse_addr(true);
    }

    template<typename OpenHandler, typename CloseHandler, typename MessageHandler>
    server(OpenHandler openHandler,
           CloseHandler closeHandler,
           MessageHandler messageHandler):
    server{}
    {
      set_open_handler(openHandler);
      set_close_handler(closeHandler);
      set_message_handler(messageHandler);
    }

    template<typename Handler>
    void set_open_handler(Handler h)
    {
        m_server.set_open_handler(h);
    }

    template<typename Handler>
    void set_close_handler(Handler h)
    {
        m_server.set_close_handler(h);
    }

    template<typename Handler>
    void set_message_handler(Handler h)
    {
        m_server.set_message_handler([=] (connection_handler hdl, server_t::message_ptr msg)
        {
          try
          {
            send_message(hdl, h(hdl, msg->get_payload()));
          }
          catch(PathNotFoundException& e)
          {
            auto con = m_server.get_con_from_hdl(hdl);
            con->set_status(websocketpp::http::status_code::not_found);
          }
          catch(BadRequestException& e)
          {
            auto con = m_server.get_con_from_hdl(hdl);
            std::cerr << "Error in request: " << con->get_uri()->get_resource() << " ==> "<< e.what() << std::endl;
            con->set_status(websocketpp::http::status_code::bad_request);
          }
        });

        m_server.set_http_handler([=] (connection_handler hdl)
        {
          auto con = m_server.get_con_from_hdl(hdl);

          con->replace_header("Content-Type", "application/json; charset=utf-8");
          try
          {
            con->set_body(h(hdl, con->get_uri()->get_resource()) + "\0");
            con->set_status(websocketpp::http::status_code::ok);
          }
          catch(PathNotFoundException& e)
          {
            con->set_status(websocketpp::http::status_code::not_found);
          }
          catch(BadRequestException& e)
          {
            std::cerr << "Error in request: " << con->get_uri()->get_resource() << " ==> "<< e.what() << std::endl;
            con->set_status(websocketpp::http::status_code::bad_request);
          }
        });
    }

    void run(uint16_t port = 9002)
    {
      m_server.listen(port);
      m_server.start_accept();
      m_server.run();
    }

    void send_message(connection_handler hdl, const std::string& message)
    {
      auto con = m_server.get_con_from_hdl(hdl);
      con->send(message);
    }

  private:
    server_t m_server;
};

}
}
