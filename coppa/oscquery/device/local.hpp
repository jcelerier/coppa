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
/**
 * @brief The local_device class
 *
 * An oscquery-compliant server.
 */
class local_device : public coppa::local_device<
    coppa::locked_map<coppa::oscquery::basic_map<coppa::oscquery::ParameterMap>>,
    coppa::ws::server,
    coppa::oscquery::query_parser,
    coppa::oscquery::answerer,
    coppa::oscquery::json::writer,
    coppa::osc::receiver,
    coppa::osc::message_handler
    >
{ };


/**
 * @brief The synchronizing_local_device class
 *
 * An oscquery-compliant server that syncs itself to the clients.
 */
class synchronizing_local_device : public coppa::synchronizing_local_device<
    coppa::locked_map<coppa::oscquery::basic_map<coppa::oscquery::ParameterMap>>,
    coppa::ws::server,
    coppa::oscquery::query_parser,
    coppa::oscquery::answerer,
    coppa::oscquery::json::writer,
    coppa::osc::receiver,
    coppa::osc::message_handler
    >
{ };
}
}
