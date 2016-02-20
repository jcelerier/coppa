#pragma once
#include <coppa/minuit/parameter.hpp>
#include <coppa/protocol/osc/oscreceiver.hpp>
#include <oscpack/osc/OscTypes.h>
#include <coppa/string_view.hpp>

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
    static void on_messageReceived(
        Device& dev,
        Map& map,
        const oscpack::ReceivedMessage& m)
    {
      using namespace coppa;
      using coppa::ossia::Parameter;
      using eggs::variants::get;

      Values current_parameter;
      string_view address = m.AddressPattern();
      // Little dance for thread-safe access to the current value
      {
        auto&& l = map.acquire_read_lock();
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
};
}
}
