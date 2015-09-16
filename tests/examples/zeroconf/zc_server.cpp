#include <coppa/oscquery/device/local.hpp>
#include <coppa/protocol/websockets/server.hpp>
#include <coppa/tools/random.hpp>
#include <chrono>
#include <QCoreApplication>
#include <coppa/device/zeroconf_server.hpp>
class zc_synchronizing_local_device : public coppa::SynchronizingLocalDevice<
    coppa::LockedParameterMap<coppa::oscquery::SimpleParameterMap<coppa::oscquery::ParameterMap>>,
    coppa::BonjourServer<coppa::WebSocketServer>,
    coppa::oscquery::query_parser,
    coppa::oscquery::answerer,
    coppa::oscquery::JSON::writer,
    coppa::osc::receiver,
    coppa::osc::message_handler
    >
{
  public:
    using coppa::SynchronizingLocalDevice<
    coppa::LockedParameterMap<coppa::oscquery::SimpleParameterMap<coppa::oscquery::ParameterMap>>,
    coppa::BonjourServer<coppa::WebSocketServer>,
    coppa::oscquery::query_parser,
    coppa::oscquery::answerer,
    coppa::oscquery::JSON::writer,
    coppa::osc::receiver,
    coppa::osc::message_handler
    >::SynchronizingLocalDevice;
};


int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    using namespace coppa;

    using query_server = coppa::BonjourServer<coppa::WebSocketServer>;
    zc_synchronizing_local_device dev(query_server("A very nice device", 9002));
    setup_basic_map(dev.map());

    dev.expose();

    return app.exec();
}
