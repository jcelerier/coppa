#pragma once
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/common/thread.hpp>

namespace coppa
{
class WebSocketClient
{
  public:
    using connection_handler = websocketpp::connection_hdl;

    template<typename MessageHandler>
    WebSocketClient(MessageHandler&& messageHandler) :
      m_open{false},
      m_done{false}
    {
      m_client.clear_access_channels(websocketpp::log::alevel::all);
      m_client.init_asio();

      m_client.set_open_handler([=] (connection_handler hdl)
      {
        scoped_lock guard(m_lock);
        m_open = true;
      });
      m_client.set_message_handler([=] (connection_handler hdl, client::message_ptr msg)
      { messageHandler(hdl, msg->get_payload()); });
    }

    ~WebSocketClient()
    {
      if(m_open)
        stop();

      if(asio_thread.joinable())
        asio_thread.join();
    }

    bool connected() const
    { return m_open; }

    void stop()
    {
      scoped_lock guard(m_lock);
      m_client.close(m_hdl, websocketpp::close::status::normal, "");
      m_open = false;
    }

    std::string connect(const std::string & uri)
    {
      websocketpp::lib::error_code ec;
      auto con = m_client.get_connection(uri, ec);
      if (ec)
      {
        m_client.get_alog().write(websocketpp::log::alevel::app,
                                  "Get Connection Error: " + ec.message());
        return "";
      }

      m_hdl = con->get_handle();
      m_client.connect(con);

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      asio_thread = websocketpp::lib::thread{&client::run, &m_client};
      return con->get_host();
    }

    void sendMessage(const std::string& request)
    {
      if(!m_open)
        return;

      websocketpp::lib::error_code ec;

      m_client.send(m_hdl, request, websocketpp::frame::opcode::text, ec);

      if (ec)
      {
        m_client.get_alog().write(websocketpp::log::alevel::app,
                                  "Send Error: "+ec.message());
      }
    }

  private:
    using client = websocketpp::client<websocketpp::config::asio_client>;
    using scoped_lock = websocketpp::lib::lock_guard<websocketpp::lib::mutex>;

    websocketpp::lib::thread asio_thread;
    client m_client;
    connection_handler m_hdl;
    websocketpp::lib::mutex m_lock;
    bool m_open;
    bool m_done;
};

}
