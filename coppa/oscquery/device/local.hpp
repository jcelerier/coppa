#pragma once
#include <coppa/device/local.hpp>

#include <coppa/protocol/osc/oscreceiver.hpp>
#include <coppa/protocol/osc/oscsender.hpp>
#include <coppa/protocol/osc/oscmessagegenerator.hpp>

#include <coppa/protocol/websockets/server.hpp>

#include <coppa/oscquery/device/query_parser.hpp>
#include <coppa/oscquery/map.hpp>
#include <coppa/oscquery/json/writer.hpp>
#include <coppa/oscquery/device/query_answerer.hpp>
#include <coppa/oscquery/device/message_handler.hpp>

namespace coppa
{
namespace oscquery
{
class local_device : public coppa::LocalDevice<
    coppa::LockedParameterMap<coppa::oscquery::SimpleParameterMap<coppa::oscquery::ParameterMap>>,
    coppa::WebSocketServer,
    coppa::oscquery::query_parser,
    coppa::oscquery::answerer,
    coppa::oscquery::JSON::writer,
    coppa::osc::receiver,
    coppa::osc::message_handler
    >
{ };

class synchronizing_local_device : public coppa::SynchronizingLocalDevice<
    coppa::LockedParameterMap<coppa::oscquery::SimpleParameterMap<coppa::oscquery::ParameterMap>>,
    coppa::WebSocketServer,
    coppa::oscquery::query_parser,
    coppa::oscquery::answerer,
    coppa::oscquery::JSON::writer,
    coppa::osc::receiver,
    coppa::osc::message_handler
    >
{ };
}
}
