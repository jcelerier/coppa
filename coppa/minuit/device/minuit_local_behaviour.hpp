#pragma once
#include <coppa/minuit/device/minuit_common.hpp>
#include <coppa/string_view.hpp>
#include <oscpack/osc/OscReceivedElements.h>
#include <coppa/minuit/device/minuit_name_table.hpp>
#include <coppa/minuit/parameter_ostream.hpp>
namespace coppa
{
namespace ossia
{
// The "local" behaviour only answers to requests.
template<minuit_command Req, minuit_operation Op>
struct minuit_local_behaviour
{

    template<typename Device, typename Map>
    auto operator()(Device& dev, Map& map, const oscpack::ReceivedMessage& mess)
    {
      // By default do nothing
    }
};

// Get
template<>
struct minuit_local_behaviour<
    minuit_command::Request,
    minuit_operation::Get>
{
    template<typename Device, typename Map>
    auto operator()(Device& dev, Map& map, const oscpack::ReceivedMessage& mess)
    {
      string_view full_address{mess.ArgumentsBegin()->AsString()};
      auto idx = full_address.find_first_of(":");

      if(idx == std::string::npos)
      {
        // Value
        auto it = map.find(full_address);
        if(it != map.end())
        {
          dev.sender.send(dev.nameTable.get_action(minuit_action::GetReply),
                          full_address,
                          static_cast<const Values&>(*it)
                          );
        }
      }
      else
      {
        string_view address{full_address.data(), idx};

        // Note : bug if address == "foo:"
        auto attr = get_attribute(
                      string_view(
                        address.data() + idx + 1,
                        full_address.size() - idx - 1));

        auto it = map.find(address);
        if(it != map.end())
        {
          switch(attr)
          {
            case minuit_attribute::Value:
              dev.sender.send(dev.nameTable.get_action(minuit_action::GetReply),
                              full_address,
                              static_cast<const Values&>(*it)
                              );
              break;
            case minuit_attribute::Type:
              dev.sender.send(dev.nameTable.get_action(minuit_action::GetReply),
                              full_address,
                              to_minuit_type_text(*it)
                              );
              break;
            case minuit_attribute::RangeBounds:
              dev.sender.send(dev.nameTable.get_action(minuit_action::GetReply),
                              full_address,
                              static_cast<const Range&>(*it)
                              );
              break;
            case minuit_attribute::RangeClipMode:
              dev.sender.send(dev.nameTable.get_action(minuit_action::GetReply),
                              full_address,
                              to_minuit_bounding_text(it->bounding)
                              );
              break;
            case minuit_attribute::RepetitionFilter:
              dev.sender.send(dev.nameTable.get_action(minuit_action::GetReply),
                              full_address,
                              it->repetitionFilter
                              );
              break;
            case minuit_attribute::Service:
              dev.sender.send(dev.nameTable.get_action(minuit_action::GetReply),
                              full_address,
                              to_minuit_service_text(it->access)
                              );
              break;
            case minuit_attribute::Priority:
            case minuit_attribute::Description:
            default:
              break;
          }
        }
      }
    }
};

// Listen
template<>
struct minuit_local_behaviour<
    minuit_command::Request,
    minuit_operation::Listen>
{
    template<typename Device, typename Map>
    auto operator()(
        Device& dev,
        Map& map,
        const oscpack::ReceivedMessage& mess)
    {
      // Add the address to the listeners
    }
};

// Namespace
template<>
struct minuit_local_behaviour<
    minuit_command::Request,
    minuit_operation::Namespace>
{
    template<typename Device, typename Children>
    void handle_root(
        Device& dev,
        Children&& c)
    {
      dev.sender.send(dev.nameTable.get_action(minuit_action::NamespaceReply),
                      "/",
                      "Application",
                      "nodes={",
                               c,
                            "}",
                      "attributes={",
                                 "}");

    }

    template<typename Device, typename Children>
    void handle_container(
        Device& dev,
        string_view address,
        Children&& c)
    {
      dev.sender.send(dev.nameTable.get_action(minuit_action::NamespaceReply),
                      address,
                      "Container",
                      "nodes={",
                               c,
                            "}",
                      "attributes={",
                                 "}");

    }

    template<typename Device>
    void handle_data(
        Device& dev,
        string_view address)
    {
      dev.sender.send(dev.nameTable.get_action(minuit_action::NamespaceReply),
                      address,
                      "Data",
                      "attributes={",
                                    "rangeBounds"      ,
                                    "rangeClipmode"    ,
                                    "type"             ,
                                    "repetitionsFilter",
                                    "service"          ,
                                    "priority"         ,
                                    "value"            ,
                                 "}");

    }

    template<typename Device, typename Map>
    auto operator()(
        Device& dev,
        Map& map,
        const oscpack::ReceivedMessage& mess)
    {
      string_view address{mess.ArgumentsBegin()->AsString()};
      if(isRoot(address))
      {
        handle_root(dev, get_children_names(map, address));
      }
      else
      {
        auto cld = get_children_names(map, address);
        if(!cld.empty())
        {
          handle_container(dev, address, cld);
        }
        else
        {
          handle_data(dev, address);
        }
      }
    }
};

}
}
