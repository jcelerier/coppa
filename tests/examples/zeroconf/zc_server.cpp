#include <coppa/oscquery/device/local.hpp>
#include <coppa/protocol/websockets/server.hpp>
#include <coppa/tools/random.hpp>
#include <chrono>
#include <QCoreApplication>
#include <coppa/device/zeroconf_server.hpp>
class zc_synchronizing_local_device : public coppa::synchronizing_local_device<
    coppa::locked_map<coppa::oscquery::basic_map<coppa::oscquery::ParameterMap>>,
    coppa::zeroconf_server<coppa::ws::server>,
    coppa::oscquery::query_parser,
    coppa::oscquery::answerer,
    coppa::oscquery::json::writer,
    coppa::osc::receiver,
    coppa::osc::message_handler
    >
{
  public:
    using coppa::synchronizing_local_device<
    coppa::locked_map<coppa::oscquery::basic_map<coppa::oscquery::ParameterMap>>,
    coppa::zeroconf_server<coppa::ws::server>,
    coppa::oscquery::query_parser,
    coppa::oscquery::answerer,
    coppa::oscquery::json::writer,
    coppa::osc::receiver,
    coppa::osc::message_handler
    >::synchronizing_local_device;
};


int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    using namespace coppa;

    using query_server = coppa::zeroconf_server<coppa::ws::server>;
    zc_synchronizing_local_device dev(query_server("A very nice device", 9002));
    setup_basic_map(dev);

    dev.expose();

    return app.exec();
}
