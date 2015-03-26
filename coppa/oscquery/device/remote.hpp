#pragma once
#include <coppa/oscquery/websockets.hpp>
#include <coppa/osc/oscreceiver.hpp>
#include <coppa/osc/oscsender.hpp>
#include <coppa/osc/oscmessagegenerator.hpp>
#include <unordered_set>
#include <coppa/oscquery/json/parser.hpp>
#include <coppa/device/remote.hpp>
namespace coppa
{
namespace oscquery
{

class RemoteDevice : public QueryRemoteDevice<
    ParameterMap,
    JSONParser,
    RemoteQueryClient<WebSocketClient, JSONParser>,
    SettableMap<ParameterMap, OscSender>>
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
