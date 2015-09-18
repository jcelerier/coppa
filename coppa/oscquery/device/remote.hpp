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
    basic_map<ParameterMap>,
    json::parser,
    remote_query_client<ws::client, json::parser>,
    remote_map_setter<basic_map<ParameterMap>, osc::sender>>
{
  public:
    using remote_query_device::remote_query_device;
    void set(const std::string& addr, oscquery::Values& val)
    {
      remote_map_setter::set(addr, val.values);
    }
};
}
}
