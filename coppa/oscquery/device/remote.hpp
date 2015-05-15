#pragma once
#include <coppa/oscquery/websockets.hpp>
#include <coppa/protocol/osc/oscsender.hpp>
#include <coppa/protocol/osc/oscmessagegenerator.hpp>
#include <unordered_set>
#include <coppa/oscquery/json/parser.hpp>
#include <coppa/device/remote.hpp>
#include <coppa/protocol/websockets/client.hpp>
namespace coppa
{
namespace oscquery
{

class RemoteDevice : public QueryRemoteDevice<
    SimpleParameterMap<ParameterMap>,
    JSONParser,
    RemoteQueryClient<WebSocketClient, JSONParser>,
    SettableMap<SimpleParameterMap<ParameterMap>, OscSender>>
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
