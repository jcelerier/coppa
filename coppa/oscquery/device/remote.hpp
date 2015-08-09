#pragma once
#include <coppa/oscquery/websockets.hpp>
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
// Maybe a better name would be "mirror" ?
class RemoteDevice : public QueryRemoteDevice<
    SimpleParameterMap<ParameterMap>,
    JSON::parser,
    RemoteQueryClient<WebSocketClient, JSON::parser>,
    SettableMap<SimpleParameterMap<ParameterMap>, osc::sender>>
{
  public:
    using QueryRemoteDevice::QueryRemoteDevice;
    void set(const std::string& addr, oscquery::Values& val)
    {
      SettableMap::set(addr, val.values);
    }
};
}
}
