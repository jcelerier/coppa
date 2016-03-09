#pragma once
#include <oscpack/osc/OscReceivedElements.h>
#include <coppa/ossia/parameter.hpp>
#include <oscpack/osc/OscTypesTraits.h>
#include <coppa/exceptions/BadRequest.hpp>

namespace coppa
{
namespace ossia
{

template<typename T>
void convert_numeric(oscpack::ReceivedMessageArgument arg, T& val)
{
  using namespace oscpack;
  switch(arg.TypeTag())
  {
    case TRUE_TYPE_TAG:
      val = convert<TRUE_TYPE_TAG>(arg);
      break;
    case FALSE_TYPE_TAG:
      val = convert<FALSE_TYPE_TAG>(arg);
      break;
    case INT32_TYPE_TAG:
      val = convert<INT32_TYPE_TAG>(arg);
      break;
    case INT64_TYPE_TAG:
      val = convert<INT64_TYPE_TAG>(arg);
      break;
    case FLOAT_TYPE_TAG:
      val = convert<FLOAT_TYPE_TAG>(arg);
      break;
    case DOUBLE_TYPE_TAG:
      val = convert<DOUBLE_TYPE_TAG>(arg);
      break;
    case CHAR_TYPE_TAG:
      val = convert<CHAR_TYPE_TAG>(arg);
      break;
    default:
      break;
      // Do nothing
  }
}

inline void convert_string(oscpack::ReceivedMessageArgument arg, std::string& val)
{
  using namespace oscpack;
  switch(arg.TypeTag())
  {
    case STRING_TYPE_TAG:
      val = convert<STRING_TYPE_TAG>(arg);
      break;
    case SYMBOL_TYPE_TAG:
      val = convert<SYMBOL_TYPE_TAG>(arg);
      break;
    default:
      break;
      // Do nothing
  }
}

inline void convert_tuple(
    oscpack::ReceivedMessageArgumentIterator it,
    oscpack::ReceivedMessageArgumentIterator end_it,
    Tuple& tuple)
{
  using namespace oscpack;

  // Setup
  tuple.variants.clear();

  // Handle single value case
  for(;it != end_it; ++it)
  {
    auto& arg = *it;
    auto type = arg.TypeTag();

    switch(type)
    {
      // TODO nil / impulse ?
      case TRUE_TYPE_TAG:
        tuple.variants.push_back(convert<TRUE_TYPE_TAG>(arg));
        break;
      case FALSE_TYPE_TAG:
        tuple.variants.push_back(convert<FALSE_TYPE_TAG>(arg));
        break;
      case INT32_TYPE_TAG:
        tuple.variants.push_back(convert<INT32_TYPE_TAG>(arg));
        break;
      case INT64_TYPE_TAG:
        tuple.variants.push_back(int32_t(convert<INT64_TYPE_TAG>(arg)));
        break;
      case FLOAT_TYPE_TAG:
        tuple.variants.push_back(convert<FLOAT_TYPE_TAG>(arg));
        break;
      case DOUBLE_TYPE_TAG:
        tuple.variants.push_back(float(convert<DOUBLE_TYPE_TAG>(arg)));
        break;
      case CHAR_TYPE_TAG:
        tuple.variants.push_back(convert<CHAR_TYPE_TAG>(arg));
        break;
      case STRING_TYPE_TAG:
        tuple.variants.push_back(convert<STRING_TYPE_TAG>(arg));
        break;
      case SYMBOL_TYPE_TAG:
        tuple.variants.push_back(convert<SYMBOL_TYPE_TAG>(arg));
        break;
      default:
        break;
    }
  }
}

inline void convert_single_value(
    oscpack::ReceivedMessageArgumentIterator arg,
    Variant& elt)
{
  using namespace oscpack;
  using namespace eggs::variants;
  struct vis {
      oscpack::ReceivedMessageArgumentIterator it;

      void operator()(None&) const {
      }
      void operator()(Impulse&) const {
      }
      void operator()(int32_t& val) const {
        convert_numeric<int32_t>(*it, val);
      }
      void operator()(float& val) const {
        convert_numeric<float>(*it, val);
      }
      void operator()(bool& val) const {
        convert_numeric<bool>(*it, val);
      }
      void operator()(char& val) const {
        convert_numeric<char>(*it, val);
      }
      void operator()(std::string& val) const {
        convert_string(*it, val);
      }
      void operator()(Tuple&) const {
        throw;
      }
      void operator()(Generic& val) const {
        int n = 0;
        const char* data{};
        it->AsBlob(reinterpret_cast<const void*&>(data), n);
        val = coppa::Generic{std::string(data, n)};
      }

  } visitor{arg};

  eggs::variants::apply(visitor, elt);
}

inline coppa::ossia::Value read_value(
    oscpack::ReceivedMessageArgumentIterator it,
    oscpack::ReceivedMessageArgumentIterator end_it,
    coppa::ossia::Value& source)
{
  auto cur_type = coppa::ossia::which(source.value);
  if(cur_type == Type::none_t || cur_type == Type::impulse_t)
    return source;

  if(it != end_it)
  {
    if(cur_type != coppa::ossia::Type::tuple_t)
    {
      convert_single_value(it, source.value);
    }
    else
    {
      convert_tuple(it, end_it, eggs::variants::get<Tuple>(source.value));
    }
  }
  // If it == end_it then we have an impulse.

  return source;
}

}
}
