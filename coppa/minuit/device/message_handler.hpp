#pragma once
#include <coppa/minuit/parameter.hpp>
#include <coppa/protocol/osc/oscreceiver.hpp>
#include <oscpack/osc/OscTypes.h>
namespace coppa
{
namespace ossia 
{
class message_handler : public coppa::osc::receiver
{
  public:
    
    void assign(
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
    
    Variant make(
        const oscpack::ReceivedMessageArgument& arg)
    {
      using namespace eggs::variants;
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
    

    template<typename Device>
    static void handleImpulse(
        Device& dev, 
        const Parameter& v,
        const oscpack::ReceivedMessage& m)
    {
      dev.update(
            m.AddressPattern(),
            [&] (auto& v) {
        v.value = Impulse{};
      });    
    }
    
    template<typename Device>
    static void handleOne(
        Device& dev, 
        const Parameter& param, 
        const oscpack::ReceivedMessage& m)
    {
      auto arg = *m.ArgumentsBegin();
      if(arg.TypeTag() == getOSCType(param.value))
      {
        dev.update(
              m.AddressPattern(),
              [&] (auto& v) {
          assign(m, v.value);
        });
      }      
    }
    
    template<typename Device>
    static void handleTuple(
        Device& dev, 
        const Values& tpl, 
        const oscpack::ReceivedMessage& m)
    { 
      tpl.clear();
      tpl.reserve(m.ArgumentCount());
            
      // If everything is okay, we can update the device.
      dev.update(
            m.AddressPattern(),
            [&] (auto& v) {
        int i = 0;

        for(auto it = m.ArgumentsBegin(); it != m.ArgumentsEnd(); ++it, ++i)
        {
          tpl.variants.push_back();
        }
      });      
    }

    template<typename Device>
    static void on_messageReceived(
        Device& dev,
        const oscpack::ReceivedMessage& m)
    {
      using namespace coppa;
      using coppa::ossia::Parameter;

      Parameter v;
      // Little dance for thread-safe access to the current value
      {
        auto&& l = dev.acquire_read_lock();
        auto node_it = dev.find(m.AddressPattern());
        if(node_it == dev.end())
          return;

        v = *node_it;
      }

      // First check the compatibility
      if(v.is<Values>())
      {
        return handleTuple(dev, v, m);
      }
      else
      {
        switch(m.ArgumentCount())
        {
          case 0:
            return handleImpulse(dev, v, m);
          case 1:
            return handleOne(dev, v, m);
          default:
            break;
        }
      }

    }
};
}
}
