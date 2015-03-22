#pragma once
#include <coppa/oscquery/parameter.hpp>
#include <coppa/oscquery/json.hpp>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/client.hpp>

#include <websocketpp/common/thread.hpp>
#include <websocketpp/http/request.hpp>

#include <boost/algorithm/string.hpp>
namespace coppa
{
    namespace oscquery
    {
        class WebSocketClient
        {
                websocketpp::lib::thread asio_thread;
            public:
                typedef websocketpp::client<websocketpp::config::asio_client> client;
                typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;

                WebSocketClient() :
                    m_open(false),
                    m_done(false)
                {
                    using namespace websocketpp::lib;
                    using namespace std::placeholders;

                    m_client.clear_access_channels(websocketpp::log::alevel::all);
                    m_client.init_asio();

                    m_client.set_open_handler(bind(&WebSocketClient::on_open, this, ::_1));
                    m_client.set_message_handler(bind(&WebSocketClient::on_message, this, ::_1, ::_2));
                }

                ~WebSocketClient()
                {
                    if(m_open)
                        stop();

                    if(asio_thread.joinable())
                        asio_thread.join();
                }

                void on_open(websocketpp::connection_hdl hdl)
                {
                    scoped_lock guard(m_lock);
                    m_open = true;
                }

                void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg)
                {
                    std::cout << std::endl << std::endl << msg->get_payload() << std::endl << std::endl;
                }

                void stop()
                {
                    scoped_lock guard(m_lock);
                    m_client.close(m_hdl, websocketpp::close::status::normal, "");
                    m_open = false;
                }

                void connect(const std::string & uri)
                {
                    websocketpp::lib::error_code ec;
                    auto con = m_client.get_connection(uri, ec);
                    if (ec)
                    {
                        m_client.get_alog().write(websocketpp::log::alevel::app,
                                                  "Get Connection Error: " + ec.message());
                        return;
                    }

                    m_hdl = con->get_handle();
                    m_client.connect(con);

                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    asio_thread = websocketpp::lib::thread{&client::run, &m_client};
                }

                void send_request(const std::string& request)
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
                client m_client;
                websocketpp::connection_hdl m_hdl;
                websocketpp::lib::mutex m_lock;
                bool m_open;
                bool m_done;
        };


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
                    using con_hdl = websocketpp::connection_hdl;
                    m_server.init_asio();

                    m_server.set_open_handler(openHandler);
                    m_server.set_close_handler(closeHandler);

                    m_server.set_message_handler([=] (con_hdl hdl, server::message_ptr msg)
                    { sendMessage(hdl, messageHandler(hdl, msg->get_payload())); });

                    m_server.set_http_handler([=] (con_hdl hdl)
                    {
                        server::connection_ptr con = m_server.get_con_from_hdl(hdl);
                        con->set_body(messageHandler(hdl, con->get_uri()->get_resource()));
                        con->set_status(websocketpp::http::status_code::ok);
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
}
