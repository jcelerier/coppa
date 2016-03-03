#pragma once
#include <coppa/minuit/device/minuit_common.hpp>
#include <coppa/minuit/device/osc_common.hpp>
#include <coppa/string_view.hpp>
#include <oscpack/osc/OscReceivedElements.h>
#include <future>
#include <iostream>

namespace coppa
{
namespace ossia
{
template<minuit_command Req, minuit_operation Op>
struct minuit_remote_behaviour
{
    template<typename Device, typename Map>
    auto operator()(Device& dev, Map& map, const oscpack::ReceivedMessage& mess)
    {
      // By default do nothing
    }
};

// Get
template<>
struct minuit_remote_behaviour<
    minuit_command::Answer,
    minuit_operation::Get>
{
    Values get_values(
        oscpack::ReceivedMessageArgumentIterator beg_it,
        oscpack::ReceivedMessageArgumentIterator end_it)
    {
      Values v;
      values_reader<
          lax_error_handler,
          conversion_mode::Replace>{}(
             beg_it,
             end_it,
             v);

      return v;
    }

    Range get_range(
        oscpack::ReceivedMessageArgumentIterator beg_it,
        oscpack::ReceivedMessageArgumentIterator end_it)
    {
      Range v;
      assert(false); // TODO
      return v;
    }


    template<typename Device, typename Map>
    auto operator()(Device& dev, Map& map, const oscpack::ReceivedMessage& mess)
    {
      auto mess_it = mess.ArgumentsBegin();
      string_view full_address{mess_it->AsString()};
      auto idx = full_address.find_first_of(":");

      if(idx == std::string::npos)
      {
        // Value
          auto res_it = map.update_attributes(
                full_address,
                this->get_values(mess_it, mess.ArgumentsEnd()));

          return res_it;
      }
      else
      {
        string_view address{full_address.data(), idx};

        // Note : bug if address == "foo:"
        auto attr = get_attribute(
                      string_view(
                        address.data() + idx + 1,
                        full_address.size() - idx - 1));

        ++mess_it;
        // mess_it is now at the first argument after the address:attribute

        switch(attr)
        {
          case minuit_attribute::Value:
            return map.update_attributes(
                  address,
                  this->get_values(mess_it, mess.ArgumentsEnd()));
            break;
          case minuit_attribute::Type:
            // default-initialize with the type
            return map.update_attributes(
                  address,
                  from_minuit_type_text(mess_it->AsString()));
            break;
          case minuit_attribute::RangeBounds:
            return map.update_attributes(
                  address,
                  this->get_range(mess_it, mess.ArgumentsEnd()));
            break;
          case minuit_attribute::RangeClipMode:
            return map.update_attributes(
                  address,
                  from_minuit_bounding_text(mess_it->AsString()));
            break;
          case minuit_attribute::RepetitionFilter:
            return map.update_attributes(
                  address,
                  RepetitionFilter{mess_it->AsBool()});
            break;
          case minuit_attribute::Service:
            return map.update_attributes(
                  address,
                  from_minuit_service_text(mess_it->AsString()));
            break;
        }

        return map.end();
      }

    }
};

// Listen
template<>
struct minuit_remote_behaviour<
    minuit_command::Answer,
    minuit_operation::Listen>
{
    template<typename Device, typename Map>
    auto operator()(Device& dev, Map& map, const oscpack::ReceivedMessage& mess)
    {

    }
};

template<typename It, typename Fun>
auto find_if(It begin, It end, Fun f)
{
  for(auto it = begin; it != end; ++it)
  {
    if(f(*it))
      return it;
  }
  return end;
}

// Namespace
template<>
struct minuit_remote_behaviour<
    minuit_command::Answer,
    minuit_operation::Namespace>
{
    template<typename Str>
    static auto get_container(
        Str s,
        oscpack::ReceivedMessageArgumentIterator beg_it,
        oscpack::ReceivedMessageArgumentIterator end_it)
    {
      std::vector<string_view> elements;
      auto nodes_beg_it = find_if(beg_it, end_it, [=] (const auto& mess) {
        return mess.IsString() && string_view(mess.AsStringUnchecked()) == s;
      });

      ++nodes_beg_it; // It will point on the first past "nodes={".
      if(nodes_beg_it == end_it)
        return elements;

      auto nodes_end_it = find_if(nodes_beg_it, end_it, [] (const auto& mess) {
        return mess.IsString() && string_view(mess.AsStringUnchecked()) == "}";
      });

      if(nodes_end_it == end_it)
        return elements;

      for(auto it = nodes_beg_it; it != nodes_end_it; ++it)
      {
        std::cerr << "received children : '" << it->AsString() << "'\n";
        elements.push_back(it->AsString());
      }

      return elements;
    }

    static auto get_nodes(
        oscpack::ReceivedMessageArgumentIterator beg_it,
        oscpack::ReceivedMessageArgumentIterator end_it
        )
    {
      return get_container("nodes={", beg_it, end_it);
    }

    static auto get_attributes(
        oscpack::ReceivedMessageArgumentIterator beg_it,
        oscpack::ReceivedMessageArgumentIterator end_it
        )
    {
      return get_container("attributes={", beg_it, end_it);
    }

    template<typename Device, typename Map>
    static auto handle_container(
        Device& dev,
        Map& map,
        string_view address,
        oscpack::ReceivedMessageArgumentIterator beg_it,
        oscpack::ReceivedMessageArgumentIterator end_it)
    {
      using namespace oscpack;
      auto sub_request = dev.nameTable.get_action(minuit_action::NamespaceRequest);

      // Get the sub-nodes
      for(auto node : get_nodes(beg_it, end_it))
      {
        // Creation of the address
        Parameter p;
        p.destination.reserve(address.size() + 1 + node.size());
        p.destination.append(address.data(), address.size());
        if(p.destination.back() != '/')
          p.destination.push_back('/');
        p.destination.append(node.data(), node.size());

        // Actual operation on the map
        map.insert(p);

        // request children
        dev.refresh(sub_request, p.destination);
      }
    }

    template<typename Device, typename Map>
    static auto handle_data(
        Device& dev,
        Map& map,
        string_view address,
        oscpack::ReceivedMessageArgumentIterator beg_it,
        oscpack::ReceivedMessageArgumentIterator end_it)
    {
      using namespace oscpack;
      auto it = map.find(address);
      if(it != map.end())
      {
        auto sub_request = dev.nameTable.get_action(minuit_action::GetRequest);

        for(auto attrib : get_attributes(beg_it, end_it))
        {
          // name?get address:attribute
          // TODO std::dynarray ?
          auto str = address.to_string();
          str.push_back(':');
          str.append(attrib.begin(), attrib.end());
          dev.sender.send(sub_request, string_view(str));
        }
      }
    }


    template<typename Device, typename Map>
    static auto handle_minuit(
        Device& dev,
        Map& map,
        string_view address,
        minuit_type type,
        oscpack::ReceivedMessageArgumentIterator beg_it,
        oscpack::ReceivedMessageArgumentIterator end_it)
    {
      switch(type)
      {
        case minuit_type::Application:
        case minuit_type::Container:
        {
          handle_container(dev, map, address, beg_it, end_it);
          break;
        }
        case minuit_type::Data:
        {
          handle_data(dev, map, address, beg_it, end_it);
          break;
        }
        case minuit_type::None:
          // just add the node ?
          break;
      }
    }

    template<typename Device, typename Map>
    auto operator()(Device& dev, Map& map, const oscpack::ReceivedMessage& mess)
    {
      auto it = mess.ArgumentsBegin();
      string_view address = it->AsString();
      auto type = get_type((++it)->AsString()[0]);

      handle_minuit(dev, map, address, type, it, mess.ArgumentsEnd());
    }
};

}
}
