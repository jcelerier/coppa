#pragma once
#include <coppa/minuit/device/minuit_common.hpp>
#include <coppa/string_view.hpp>
#include <oscpack/osc/OscReceivedElements.h>

/*
struct parsed_namespace_minuit_request
{
    string_view address_pattern;
    string_view attribute;
    boost::container::small_vector<string_view, 16> nodes;
    boost::container::small_vector<minuit_attributes, 8> attributes;
};*/

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
    template<typename Device, typename Map>
    auto operator()(Device& dev, Map& map, const oscpack::ReceivedMessage& mess)
    {

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
    auto get_nodes(
        oscpack::ReceivedMessageArgumentIterator beg_it,
        oscpack::ReceivedMessageArgumentIterator end_it
        )
    {
      std::vector<string_view> nodes;
      auto nodes_beg_it = find_if(beg_it, end_it, [] (const auto& mess) {
        return mess.IsString() && string_view(mess.AsStringUnchecked()) == "nodes={";
      });

      ++nodes_beg_it; // It will point on the first past "nodes={".
      if(nodes_beg_it == end_it)
        return nodes;

      auto nodes_end_it = find_if(nodes_beg_it, end_it, [] (const auto& mess) {
        return mess.IsString() && string_view(mess.AsStringUnchecked()) == "}";
      });

      if(nodes_end_it == end_it)
        return nodes;

      for(auto it = nodes_beg_it; it != nodes_end_it; ++it)
      {
        nodes.push_back(it->AsStringUnchecked());
      }

      return nodes;
    }

    template<typename Device, typename Map>
    auto operator()(Device& dev, Map& map, const oscpack::ReceivedMessage& mess)
    {

      auto it = mess.ArgumentsBegin();
      string_view address = it->AsString();
      auto type = get_type((++it)->AsString()[0]);
      switch(type)
      {
        case minuit_type::Application:
        case minuit_type::Container:
        {
          // Get the sub-nodes
          auto nodes = get_nodes(it, mess.ArgumentsEnd());

          for(auto node : nodes)
          {
            std::string actual_address{address.to_string()};
            if(actual_address.back() != '/')
              actual_address.push_back('/');
            actual_address.append(node.to_string());
            map.insert(actual_address);

            dev.sender.send(dev.name() + "?namespace", string_view(actual_address));
          }

          break;
        }
        case minuit_type::Data:
        {
          // Just add the node
          break;
        }
        case minuit_type::None:
          // just add the node ?
          break;
      }

      std::cerr << mess.AddressPattern() << " ";
      for(auto arg : mess)
      {
        if(arg.IsString())
        {
          std::cerr << arg.AsString() << " ";
        }
      }

      std::cerr << "\n";
    }
};

}
}
