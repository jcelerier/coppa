#pragma once
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/http/request.hpp>

#include <coppa/exceptions/BadRequest.hpp>
namespace coppa
{
class WebSocketServer
{
  public:
    using server = websocketpp::server<websocketpp::config::asio>;
    using connection_handler = websocketpp::connection_hdl;

    template<typename OpenHandler, typename CloseHandler, typename MessageHandler>
    WebSocketServer(OpenHandler&& openHandler,
                    CloseHandler&& closeHandler,
                    MessageHandler&& messageHandler)
    {
      m_server.init_asio();
      m_server.set_reuse_addr(true);

      m_server.set_open_handler(openHandler);
      m_server.set_close_handler(closeHandler);

      m_server.set_message_handler([=] (connection_handler hdl, server::message_ptr msg)
      {
        try{
        sendMessage(hdl, messageHandler(hdl, msg->get_payload()));
        }
        catch(...)
        {
          server::connection_ptr con = m_server.get_con_from_hdl(hdl);
          con->set_status(websocketpp::http::status_code::bad_request);
        }
      });

      m_server.set_http_handler([=] (connection_handler hdl)
      {
        server::connection_ptr con = m_server.get_con_from_hdl(hdl);

        con->replace_header("Content-Type", "application/json; charset=utf-8");
        try{
        con->set_body(messageHandler(hdl, con->get_uri()->get_resource()) + "\0");
        con->set_status(websocketpp::http::status_code::ok);
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

    void sendMessage(connection_handler hdl, const std::string& message)
    {
      server::connection_ptr con = m_server.get_con_from_hdl(hdl);
      con->send(message);
    }

  private:
    server m_server;
};
}
