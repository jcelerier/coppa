#pragma once
#include <coppa/protocol/osc/oscsender.hpp>
#include <coppa/protocol/osc/oscmessagegenerator.hpp>
#include <unordered_set>
#include <coppa/oscquery/json/parser.hpp>
#include <coppa/device/remote.hpp>
#include <coppa/protocol/websockets/client.hpp>
#include <coppa/oscquery/map.hpp>
namespace coppa
{
namespace oscquery
{
/**
 * @brief The remote_device class
 *
 * A device that mirrors a remote OSCQuery server.
 */
class remote_device : public remote_query_device<
    SimpleParameterMap<ParameterMap>,
    json::parser,
    remote_query_client<ws::client, json::parser>,
    SettableMap<SimpleParameterMap<ParameterMap>, osc::sender>>
{
  public:
    using remote_query_device::remote_query_device;
    void set(const std::string& addr, oscquery::Values& val)
    {
      SettableMap::set(addr, val.values);
    }
};
}
}
