#pragma once
#include <coppa/minuit/parameter.hpp>
#include <coppa/minuit/device/minuit_common.hpp>
#include <coppa/minuit/device/minuit_remote_behaviour.hpp>
#include <oscpack/osc/OscReceivedElements.h>
#include <future>

namespace coppa
{
namespace ossia
{

struct get_promise
{
        get_promise(const std::string& addr):
            address{addr} { }
        std::string address;
        std::promise<coppa::ossia::Parameter> promise;
};

template<
        template<
          minuit_command,
          minuit_operation>
        class Handler,
        minuit_command Req,
        minuit_operation Op>
struct minuit_callback_behaviour_wrapper
{
    template<typename Device, typename Map>
    auto operator()(Device& dev, Map& map, const oscpack::ReceivedMessage& mess)
    {
        Handler<Req, Op>{}(dev, map, mess);
    }
};


template<
        template<
          minuit_command,
          minuit_operation>
        class Handler>
struct minuit_callback_behaviour_wrapper<Handler, minuit_command::Answer, minuit_operation::Get>
{
    template<typename Device, typename Map>
    auto operator()(Device& dev, Map& map, const oscpack::ReceivedMessage& mess)
    {
        auto res_it = Handler<minuit_command::Answer, minuit_operation::Get>{}(dev, map, mess);

        if(res_it != map.end())
        {
            // TODO for all if instead ?
            auto prom_it = std::find_if(
                        dev.m_getPromises.begin(),
                        dev.m_getPromises.end(),
                        [=] (const auto& prom) {
               return prom.address == res_it->destination;
            });

            if(prom_it != dev.m_getPromises.end())
            {
              prom_it->promise.set_value(*res_it);
              dev.m_getPromises.erase(prom_it);
            }
        }
        // TODO here : - boost.lockfree queue for namespaces : we add the requested paths to a queue
        // when they are received we remove them
        // when the queue is empty, the namespace querying is finished (else we retry three times or something)

        // For "get" we set our promise as soon as we're done ? where shall we store them ?

        //dev.m_getPromises
    }
};

template<
  minuit_command c,
  minuit_operation op>
using minuit_callback_behaviour_wrapper_t = minuit_callback_behaviour_wrapper<minuit_remote_behaviour, c, op>;
template<
        template<
          minuit_command,
          minuit_operation>
        class Handler>
struct minuit_callback_behaviour_wrapper<Handler, minuit_command::Answer, minuit_operation::Listen>
{
    template<typename Device, typename Map>
    auto operator()(Device& dev, Map& map, const oscpack::ReceivedMessage& mess)
    {
      Handler<minuit_command::Answer, minuit_operation::Listen>{}(dev, map, mess);
    }
};

template<
        template<
          minuit_command,
          minuit_operation>
        class Handler>
struct minuit_callback_behaviour_wrapper<Handler, minuit_command::Answer, minuit_operation::Namespace>
{
    using impl_t = Handler<minuit_command::Answer, minuit_operation::Namespace>;

    template<typename Device, typename Map>
    auto operator()(Device& dev, Map& map, const oscpack::ReceivedMessage& mess)
    {
      auto it = mess.ArgumentsBegin();
      string_view address = it->AsString();
      auto type = get_type((++it)->AsString()[0]);

      impl_t::handle_minuit(dev, map, address, type, it, mess.ArgumentsEnd());

      auto ns_it = dev.m_namespaceRequests.find(address);
      if(ns_it != dev.m_namespaceRequests.end())
      {
        dev.m_namespaceRequests.erase(ns_it);
        if(dev.m_namespaceRequests.empty())
        {
          dev.m_nsPromise.set_value();
        }
      }
    }
};
}
}
