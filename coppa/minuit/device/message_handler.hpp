#pragma once
#include <coppa/minuit/parameter.hpp>
#include <coppa/map.hpp>
#include <coppa/protocol/osc/oscreceiver.hpp>
#include <oscpack/osc/OscTypes.h>
#include <coppa/string_view.hpp>
#include <boost/iterator/filter_iterator.hpp>

namespace oscpack
{
auto begin(const oscpack::ReceivedMessage& mes)
{
  return mes.ArgumentsBegin();
}
auto end(const oscpack::ReceivedMessage& mes)
{
  return mes.ArgumentsEnd();
}
}
namespace coppa
{
namespace ossia 
{
class message_handler : public coppa::osc::receiver
{
  public:    
    static void assign_checked(
        const oscpack::ReceivedMessageArgument& arg, 
        Variant& elt)
    {
      using namespace eggs::variants;
      switch(which(elt))
      {
        case Type::impulse_t:
          break;
        case Type::bool_t:
          if(arg.IsBool())
            elt = arg.AsBoolUnchecked();
          break;
        case Type::int_t:
          if(arg.IsInt32())
            elt = arg.AsInt32Unchecked();
          break;
        case Type::float_t:
          if(arg.IsFloat())
            elt = arg.AsFloatUnchecked();
          break;
        case Type::char_t:
          if(arg.IsChar())
            elt = arg.AsCharUnchecked();
          break;
        case Type::string_t:
          if(arg.IsString())
            elt = std::string(arg.AsStringUnchecked());
          break;
        case Type::tuple_t:
          break;
        case Type::generic_t:
        {
          if(arg.IsBlob())
          {
            int n = 0;
            const char* data{};
            arg.AsBlobUnchecked(reinterpret_cast<const void*&>(data), n);
            elt = coppa::Generic{std::string(data, n)};
          }
          break;
        }
        default:
          break;
      }      
    }
    
    
    static void assign_unchecked(
        const oscpack::ReceivedMessageArgument& arg, 
        Variant& elt)
    {
      using namespace eggs::variants;
      switch(which(elt))
      {
        case Type::impulse_t:
          break;
        case Type::bool_t:
          elt = arg.TypeTag() == oscpack::TRUE_TYPE_TAG;
          break;
        case Type::int_t:
          elt = arg.AsInt32Unchecked();
          break;
        case Type::float_t:
          elt = arg.AsFloatUnchecked();
          break;
        case Type::char_t:
          elt = arg.AsCharUnchecked();
          break;
        case Type::string_t:
          elt = std::string(arg.AsStringUnchecked());
          break;
        case Type::generic_t:
        {
          int n = 0;
          const char* data{};
          arg.AsBlobUnchecked(reinterpret_cast<const void*&>(data), n);
          elt = coppa::Generic{std::string(data, n)};
          break;
        }
        case Type::tuple_t:
          break;
        default:
          break;
      }      
    }
    
    static Variant make(
        const oscpack::ReceivedMessageArgument& arg)
    {
      using namespace eggs::variants;
      using namespace oscpack;
      switch(arg.TypeTag())
      {
        case INT32_TYPE_TAG:
          return arg.AsInt32Unchecked();
        case FLOAT_TYPE_TAG:
          return arg.AsFloatUnchecked();
        case TRUE_TYPE_TAG:
          return true;
        case FALSE_TYPE_TAG:
          return false;
        case CHAR_TYPE_TAG: 
          return arg.AsCharUnchecked();
        case STRING_TYPE_TAG:
          return std::string(arg.AsStringUnchecked());
        case BLOB_TYPE_TAG:
        {
          int n = 0;
          const char* data{};
          arg.AsBlobUnchecked(reinterpret_cast<const void*&>(data), n);
          return coppa::Generic{std::string(data, n)};
        }
        default:
          return Impulse{};
          break;
      }      
    }
    
    static oscpack::ReceivedMessageArgumentIterator handleArray(
        oscpack::ReceivedMessageArgumentIterator it, 
        Tuple& tuple)
    {
      using eggs::variants::get;
      // First is the '['
      char c = (++it)->TypeTag();
      int i = 0;
      while(c != oscpack::ARRAY_END_TYPE_TAG)
      {
        if(c != oscpack::ARRAY_BEGIN_TYPE_TAG)
        {
          // Value case
          assign_unchecked(*it, tuple.variants[i]);
          ++it;
        }
        else
        {
          // Tuple case.
          it = handleArray(it, get<Tuple>(tuple.variants[i]));
        }
        
        c = it->TypeTag();
        i++;
      }
      
      return ++it;
    }
    
    template<typename Device, typename Map>
    static void handleOSCMessage(
        Device& dev,
        Map& map,
        string_view address,
        const oscpack::ReceivedMessage& m)
    { 
      using namespace coppa;
      using coppa::ossia::Parameter;
      using eggs::variants::get;
      
      Values current_parameter;
      // The lock is already acquired in the parent
      {
        auto node_it = map.find(address);
        if(node_it == map.end())
          return;

        current_parameter = *node_it;
      }

      // First check the compatibility
      if(getOSCType(current_parameter) != m.TypeTags())
        return;
      
      // Then write the arguments
      auto end = m.ArgumentsEnd();
      int i = 0;
      for(auto it = m.ArgumentsBegin(); it != end; ++i)
      {
        if(!it->IsArrayBegin())
        {
          // Value
          assign_unchecked(*it, current_parameter.variants[i]);
          ++it;
        }
        else
        {
          // Tuple
          it = handleArray(it, get<Tuple>(current_parameter.variants[i]));
        }
      }
      
      dev.template update<string_view>(
            address,
            [&] (auto& v) {
        v.variants = current_parameter.variants;
      });  
    }
    
    template<typename Device, typename Map>
    static void on_messageReceived(
        Device& dev,
        Map& map,
        const oscpack::ReceivedMessage& m,
        const oscpack::IpEndpointName& ip)
    {
      handleOSCMessage(dev, map, m.AddressPattern(), m);
    }
};

enum class minuit_command : char
{ Request = '?', Answer = ':', Error = '!' };

enum class minuit_operation : char
{ Listen = 'l', Namespace = 'n', Get = 'g'};

enum class minuit_attributes 
{ Value, Type, Service, Priority, RangeBounds, Description, RepetitionFilter };

enum class minuit_type
{ Application, Container, Data, None };


minuit_command get_command(char str)
{
  switch(str) 
  {
    case '?':
    case ':': 
    case '!':
      return static_cast<minuit_command>(str);
    default : 
      throw std::runtime_error("unhandled command");
  }
}

minuit_operation get_operation(char str)
{
  switch(str) 
  {
    case 'l':
    case 'n': 
    case 'g':
      return static_cast<minuit_operation>(str);
    default : 
      throw std::runtime_error("unhandled operation");
  }
}

minuit_operation get_operation(string_view str)
{
  return get_operation(str[0]);
}

template<typename Map>
std::vector<Parameter> get_children(Map& map, string_view address)
{
  std::vector<Parameter> vec;
  vec.reserve(4);
  
  // TODO have a filter( that returns iterators instead 
  // (with lexical ordering, it can return a begin - end pair of iterators)
  auto filtered = coppa::filter(map, address);
  filtered.template get<0>().erase(address.to_string()); // TODO why to_string
  
  for(auto& child : filtered)
  {
    // There must be no slash at the end of the following address.
    string_view remaining{
      child.destination.data() + address.size() + 1, 
      child.destination.size() - (address.size() + 1)};
    
    if(!remaining.find('/'))
      vec.push_back(child);
  }
  
  return vec;
}

// The map should be locked beforehand and be ordered
template<typename Map, typename Key>
auto filter_iterators(const Map& map, Key&& addr)
{
  auto pred = [&] (auto& val) {
    return boost::starts_with(val.destination, addr);
  };

  auto addr_it = map.find(addr);
  return std::make_pair(
        boost::make_filter_iterator(pred, ++addr_it, map.end()),
        boost::make_filter_iterator(pred, map.end(), map.end()));
}


template<typename Map>
std::vector<string_view> get_root_children_names(
    Map& map)
{
  std::vector<string_view> vec;
  vec.reserve(16); // we could maybe assume ln2(map.size()) ?
  
  if(map.size() == 1)
    return vec;
  
  // everything
  // we start at begin + 1 to ignore root
  auto it = map.begin();
  for(it++; it != map.end(); ++it)
  {
    auto& child = *it;
    // There must be no slash at the end of the following address.
    string_view remaining{
          child.destination.data() + 1, 
          child.destination.size() - 1};
    
    if(remaining.find('/') == std::string::npos)
      vec.push_back(remaining);    
  }
  
  return vec;
}


template<typename Map>
std::vector<string_view> get_nonroot_children_names(
    Map& map, 
    string_view address)
{
  std::vector<string_view> vec;
  vec.reserve(16);
  
  auto filtered = filter_iterators(map, address);
  for(auto it = filtered.first; it != filtered.second; ++it)
  {
    auto& child = *it;
    // There must be no slash at the end of the following address.
    string_view remaining{
      child.destination.data() + address.size() + 1, 
      child.destination.size() - (address.size() + 1)};
    
    if(!remaining.find('/'))
      vec.push_back(remaining);
  }
  
  return vec;
}

template<typename Map>
std::vector<string_view> get_children_names(
    Map& map, 
    string_view address)
{
  // TODO use "constructed" vector, or vector of string_view
  // TODO http://howardhinnant.github.io/stack_alloc.html
  // reply with the child addresses and their attributes.
  
  if(isRoot(address))
  {
    return get_root_children_names(map);
  }
  else
  { 
    return get_nonroot_children_names(map, address);
  } 
}

struct parsed_namespace_minuit_request
{
    string_view address_pattern;
    string_view attribute;
    boost::container::small_vector<string_view, 16> nodes;
    boost::container::small_vector<minuit_attributes, 8> attributes;
};

template<minuit_command Req, minuit_operation Op>
struct handle_minuit;

// Get
template<>
struct handle_minuit<
    minuit_command::Request, 
    minuit_operation::Get>
{
    template<typename Device, typename Map>
    auto operator()(Device& dev, Map& map, const oscpack::ReceivedMessage& mess)
    {
      
    }    
};

template<>
struct handle_minuit<
    minuit_command::Answer, 
    minuit_operation::Get>
{
    template<typename Device, typename Map>
    auto operator()(Device& dev, Map& map, const oscpack::ReceivedMessage& mess)
    {
      
    }    
};


template<>
struct handle_minuit<
    minuit_command::Error, 
    minuit_operation::Get>
{
    template<typename Device, typename Map>
    auto operator()(Device& dev, Map& map, const oscpack::ReceivedMessage& mess)
    {
      
    }    
};

// Listen
template<>
struct handle_minuit<
    minuit_command::Request, 
    minuit_operation::Listen>
{
    template<typename Device, typename Map>
    auto operator()(Device& dev, Map& map, const oscpack::ReceivedMessage& mess)
    {
      
    }    
};

template<>
struct handle_minuit<
    minuit_command::Answer, 
    minuit_operation::Listen>
{
    template<typename Device, typename Map>
    auto operator()(Device& dev, Map& map, const oscpack::ReceivedMessage& mess)
    {
      
    }    
};


template<>
struct handle_minuit<
    minuit_command::Error, 
    minuit_operation::Listen>
{
    template<typename Device, typename Map>
    auto operator()(Device& dev, Map& map, const oscpack::ReceivedMessage& mess)
    {
      
    }    
};

// Namespace
template<>
struct handle_minuit<
    minuit_command::Request, 
    minuit_operation::Namespace>
{
    template<typename Device, typename Children>
    void handle_root(
        Device& dev,
        Children&& c)
    {
      dev.sender.send(dev.name() + ":namespace",
                      "/",
                      "Application", 
                      "nodes={", 
                               std::forward<Children>(c), 
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
      dev.sender.send(dev.name() + ":namespace",
                      address.data(),
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
      dev.sender.send(dev.name() + ":namespace",
                      address.data(),
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
        auto it = map.find(address);
        if(it != map.end())
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
    }
};

template<>
struct handle_minuit<
    minuit_command::Answer, 
    minuit_operation::Namespace>
{
    template<typename Device, typename Map>
    auto operator()(Device& dev, Map& map, const oscpack::ReceivedMessage& mess)
    {
      
    }    
};


template<>
struct handle_minuit<
    minuit_command::Error, 
    minuit_operation::Namespace>
{
    template<typename Device, typename Map>
    auto operator()(Device& dev, Map& map, const oscpack::ReceivedMessage& mess)
    {
      
    }    
};




// Namespace request : 
// app?namespace addr

// Namespace answer : 
// app:namespace addr minuit_type nodes={ n1 n2 n3 } attributes={ foo bar baz }
class minuit_message_handler : 
    public coppa::osc::receiver
{
  public:
    template<typename Device, typename Map>
    static void handleMinuitMessage(
        Device& dev,
        Map& map,
        string_view address,
        const oscpack::ReceivedMessage& m)
    { 
      // Look for either ':' or '?'
      auto idx = address.find_first_of(":?!");
      
      if(idx != std::string::npos)
      {
        //string_view application(address.data(), idx);
        //string_view request(address.data() + idx + 1, address.size() - application.size() - 1);
        
        //std::cerr << application << " " << request << std::endl;
        auto req = get_command(address[idx]);
        auto op = get_operation(*(address.data() + idx + 1));
        switch(req)
        {
          case minuit_command::Answer: // Receiving an answer
          {
            switch(op)
            {
              case minuit_operation::Listen:
                handle_minuit<minuit_command::Answer, minuit_operation::Listen>{}(dev, map, m);
                break;
              case minuit_operation::Get:
                handle_minuit<minuit_command::Answer, minuit_operation::Get>{}(dev, map, m);
                break;                
              case minuit_operation::Namespace:
                handle_minuit<minuit_command::Answer, minuit_operation::Namespace>{}(dev, map, m);
                break;
              default:
                break;
            }
            break;
          }
          case minuit_command::Request: // Receiving a request
          {
            switch(op)
            {
              case minuit_operation::Listen:
                handle_minuit<minuit_command::Request, minuit_operation::Listen>{}(dev, map, m);
                break;
              case minuit_operation::Get:
                handle_minuit<minuit_command::Request, minuit_operation::Get>{}(dev, map, m);
                break;                
              case minuit_operation::Namespace:
                handle_minuit<minuit_command::Request, minuit_operation::Namespace>{}(dev, map, m);
                break;
              default:
                break;
            }
            break;
          }            
          case minuit_command::Error: // Receiving an error
          {
            switch(op)
            {
              case minuit_operation::Listen:
                handle_minuit<minuit_command::Error, minuit_operation::Listen>{}(dev, map, m);
                break;
              case minuit_operation::Get:
                handle_minuit<minuit_command::Error, minuit_operation::Get>{}(dev, map, m);
                break;                
              case minuit_operation::Namespace:
                handle_minuit<minuit_command::Error, minuit_operation::Namespace>{}(dev, map, m);
                break;
              default:
                break;
            }
            break;
          }  
          default:
            break;
        }
      }
      
      // For minuit address, the request is the address pattern, 
      // and the arugments may contain the address of type 's', and the OSC stuff.
    }
    
    template<typename Device, typename Map>
    static void on_messageReceived(
        Device& dev,
        Map& map,
        const oscpack::ReceivedMessage& m,
        const oscpack::IpEndpointName& ip)
    {
      auto l = map.acquire_read_lock();      
      string_view address{m.AddressPattern()};
      std::cerr << "received " << address << " " << m.ArgumentsBegin()->AsString() << "\n";
      // We have to check if it's a plain osc address, or a Minuit request address.
      
      if(address.size() > 0 && address[0] == '/')
      {
        message_handler::handleOSCMessage(dev, map.get_data_map(), address, m);
      }
      else
      {
        handleMinuitMessage(dev, map.get_data_map(), address, m);
      }
    }
};
}
}
