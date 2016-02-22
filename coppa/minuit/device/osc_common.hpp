#pragma once
#include <oscpack/osc/OscReceivedElements.h>
#include <coppa/minuit/parameter.hpp>
#include <oscpack/osc/OscTypesTraits.h>

namespace coppa
{
namespace ossia
{

struct lax_error_handler
{
    template<typename T>
    auto operator()(T t) {
      return t;
    }
};

struct strict_error_handler
{
    template<typename T>
    auto operator()(T t) {
      throw;
      return t;
    }
};

template<typename ErrorHandler, typename T>
T convert_numeric(const oscpack::ReceivedMessageArgument& arg)
{
  using namespace oscpack;
  switch(arg.TypeTag())
  {
    case TRUE_TYPE_TAG:
      return convert<TRUE_TYPE_TAG>(arg);
    case FALSE_TYPE_TAG:
      return convert<FALSE_TYPE_TAG>(arg);
    case INT32_TYPE_TAG:
      return convert<INT32_TYPE_TAG>(arg);
    case INT64_TYPE_TAG:
      return convert<INT64_TYPE_TAG>(arg);
    case FLOAT_TYPE_TAG:
      return convert<FLOAT_TYPE_TAG>(arg);
    case DOUBLE_TYPE_TAG:
      return convert<DOUBLE_TYPE_TAG>(arg);
    case CHAR_TYPE_TAG:
      return convert<CHAR_TYPE_TAG>(arg);
    default:
      return ErrorHandler{}(T{});
  }
}

template<typename ErrorHandler>
std::string convert_string(oscpack::ReceivedMessageArgument arg)
{
  using namespace oscpack;
  switch(arg.TypeTag())
  {
    case STRING_TYPE_TAG:
      return convert<STRING_TYPE_TAG>(arg);
    case SYMBOL_TYPE_TAG:
      return convert<SYMBOL_TYPE_TAG>(arg);
    default:
      return ErrorHandler{}(std::string{});
  }
}



inline oscpack::ReceivedMessageArgumentIterator skipArray(
    oscpack::ReceivedMessageArgumentIterator it)
{
  using eggs::variants::get;
  // First is the '['
  char c = (++it)->TypeTag();
  while(c != oscpack::ARRAY_END_TYPE_TAG)
  {
    if(c != oscpack::ARRAY_BEGIN_TYPE_TAG)
    {
      // Value case
      ++it;
    }
    else
    {
      // Tuple case.
      it = skipArray(it);
    }

    c = it->TypeTag();
  }

  // At this point it is on ']'

  return ++it;

}

template<typename Fun>
oscpack::ReceivedMessageArgumentIterator read_array(
    Fun& f,
    oscpack::ReceivedMessageArgumentIterator it,
    Tuple& tuple)
{
  using eggs::variants::get;
  // First is the '['
  char c = (++it)->TypeTag();
  int i = 0;
  while(c != oscpack::ARRAY_END_TYPE_TAG)
  {
    if(tuple.variants.size() >= i)
    {
      it = f(it, tuple.variants[i]);
    }
    else
    {
      tuple.variants.resize(tuple.variants.size() + 1);
      it = f(it, tuple.variants.back());
    }

    c = it->TypeTag();
    i++;
  }

  // At this point it is on ']'

  return ++it;
}

template<typename ErrorHandler,
         typename Fun>
oscpack::ReceivedMessageArgumentIterator convert_tuple(
    Fun& fun,
    oscpack::ReceivedMessageArgumentIterator it,
    Tuple& tuple)
{
  using namespace oscpack;

  // Setup
  tuple.variants.clear();

  auto& arg = *it;
  auto type = arg.TypeTag();

  if(type == ARRAY_BEGIN_TYPE_TAG)
  {
    // Handle tuple case
    return read_array(fun, it, tuple);
  }
  else
  {
    // Handle single value case
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
        return ErrorHandler{}(++it);
    }
    return ++it;
  }
}

// Three possible modes :
// strict-convert :: converts the incoming value to the local type
// -> possible sub-types : if no easy conversion, keep current value, or replace with default-init ?
// strict-ignore : ignore the incoming value if it is not convertible
// strict-pre checked : we checked beforehand that the message layout was the same than the
//                      Values layout and can safely convert without checks.
// replace : replace the local type to the incoming value
enum class ConversionMode
{ StrictConvert, StrictIgnore, StrictPreChecked, Replace  };

template<typename ErrorHandler, ConversionMode>
struct ValueMaker;

template<typename ErrorHandler>
struct ValueMaker<ErrorHandler, ConversionMode::StrictConvert>
{
    auto operator()(
        oscpack::ReceivedMessageArgumentIterator arg,
        Variant& elt)
    {
      using namespace oscpack;
      using namespace eggs::variants;
      struct vis {
          ValueMaker<ErrorHandler, ConversionMode::StrictConvert>& parent;
          oscpack::ReceivedMessageArgumentIterator& it;

          using return_type = void;
          return_type operator()(None& val) const {
            // TODO error handling ?
            ++it;
          }
          return_type operator()(Impulse& val) const {
            // TODO error handling ?
            ++it;
          }

          return_type operator()(int32_t& val) const {
            val = convert_numeric<ErrorHandler, int32_t>(*it);
            ++it;
          }
          return_type operator()(float& val) const {
            val = convert_numeric<ErrorHandler, float>(*it);
            ++it;
          }
          return_type operator()(bool& val) const {
            val = convert_numeric<ErrorHandler, bool>(*it);
            ++it;
          }
          return_type operator()(char& val) const {
            val = convert_numeric<ErrorHandler, char>(*it);
            ++it;
          }
          return_type operator()(std::string& val) const {
            val = convert_string<ErrorHandler>(*it);
            ++it;
          }
          return_type operator()(Tuple& t) const {
            t.variants.clear();
            it = convert_tuple<ErrorHandler>(parent, it, t);
          }
          return_type operator()(Generic& val) const {
            int n = 0;
            const char* data{};
            it->AsBlob(reinterpret_cast<const void*&>(data), n);
            val = coppa::Generic{std::string(data, n)};
            ++it;
          }

      } visitor{*this, arg};

      eggs::variants::apply(visitor, elt);
      return visitor.it;
    }
};


template<typename ErrorHandler>
struct ValueMaker<ErrorHandler, ConversionMode::StrictPreChecked>
{
    auto operator()(
        oscpack::ReceivedMessageArgumentIterator arg,
        Variant& elt)
    {
      using namespace oscpack;
      using namespace eggs::variants;
      struct vis {
          ValueMaker<ErrorHandler, ConversionMode::StrictPreChecked>& parent;
          oscpack::ReceivedMessageArgumentIterator& it;

          using return_type = void;
          return_type operator()(None& val) const {
            ++it;
          }

          return_type operator()(Impulse& val) const {
            ++it;
          }

          return_type operator()(int32_t& val) const {
            val = it->AsInt32Unchecked();
            ++it;
          }

          return_type operator()(float& val) const {
            val = it->AsFloatUnchecked();
            ++it;
          }

          return_type operator()(bool& val) const {
            val = it->AsBoolUnchecked();
            ++it;
          }

          return_type operator()(char& val) const {
            val = it->AsCharUnchecked();
            ++it;
          }

          return_type operator()(std::string& val) const {
            val = it->AsStringUnchecked();
            ++it;
          }

          return_type operator()(Tuple& t) const {
            it = read_array(parent, it, t);
            // this one can be optimized since we know for sure that
            // the size is always correct
          }

          return_type operator()(Generic& val) const {
            int n = 0;
            const char* data{};
            it->AsBlobUnchecked(reinterpret_cast<const void*&>(data), n);
            val = coppa::Generic{data, n};
            ++it;
          }

      } visitor{*this, arg};

      eggs::variants::apply(visitor, elt);
      return visitor.it;
    }
};


template<typename ErrorHandler>
struct ValueMaker<ErrorHandler, ConversionMode::StrictIgnore>
{
    auto operator()(
        oscpack::ReceivedMessageArgumentIterator arg,
        Variant& elt)
    {
      using namespace oscpack;
      using namespace eggs::variants;
      struct vis {
          ValueMaker<ErrorHandler, ConversionMode::StrictIgnore>& parent;
          oscpack::ReceivedMessageArgumentIterator& it;

          using return_type = void;
          return_type operator()(None& val) const {
            ++it;
          }

          return_type operator()(Impulse& val) const {
            ++it;
          }

          return_type operator()(int32_t& val) const {
            if(it->IsInt32())
              val = it->AsInt32Unchecked();
            ++it;
          }

          return_type operator()(float& val) const {
            if(it->IsFloat())
              val = it->AsFloatUnchecked();
            ++it;
          }

          return_type operator()(bool& val) const {
            if(it->IsBool())
              val = it->AsBoolUnchecked();
            ++it;
          }

          return_type operator()(char& val) const {
            if(it->IsChar())
              val = it->AsCharUnchecked();
            ++it;
          }

          return_type operator()(std::string& val) const {
            if(it->IsString())
              val = it->AsStringUnchecked();
            ++it;
          }

          return_type operator()(Tuple& t) const {
            if(it->IsArrayBegin())
            {
              it = read_array(parent, it, t);
            }
            else
            {
              // Move the iterator to the end of the array
              it = skipArray(it);
            }
          }

          return_type operator()(Generic& val) const {
            if(it->IsBlob())
            {
              int32_t n = 0;
              const char* data{};
              it->AsBlobUnchecked(reinterpret_cast<const void*&>(data), n);
              val.buf = std::string{data, std::size_t(n)};
            }
            ++it;
          }

      } visitor{*this, arg};

      eggs::variants::apply(visitor, elt);
      return visitor.it;
    }
};


template<typename ErrorHandler>
struct ValueMaker<ErrorHandler, ConversionMode::Replace>
{
    auto operator()(
        oscpack::ReceivedMessageArgumentIterator it,
        Variant& elt)
    {
      using namespace eggs::variants;
      using namespace oscpack;

      auto& arg = *it;
      switch(arg.TypeTag())
      {
        case INT32_TYPE_TAG:
          elt = arg.AsInt32Unchecked();
        case FLOAT_TYPE_TAG:
          elt = arg.AsFloatUnchecked();
        case TRUE_TYPE_TAG:
          elt = true;
        case FALSE_TYPE_TAG:
          elt = false;
        case CHAR_TYPE_TAG:
          elt = arg.AsCharUnchecked();
        case STRING_TYPE_TAG:
          elt = std::string(arg.AsStringUnchecked());
        case oscpack::ARRAY_BEGIN_TYPE_TAG:
        {
          Tuple t;
          it = read_array(*this, it, t);
          elt = t;
          break;
        }
        case BLOB_TYPE_TAG:
        {
          int n = 0;
          const char* data{};
          arg.AsBlobUnchecked(reinterpret_cast<const void*&>(data), n);
          elt = coppa::Generic{std::string(data, n)};
        }
          // Arrays should be handled elsewhere
        default:
          elt = Impulse{}; // ? or not ?
          break;
      }
    }
};

/*
template<typename ErrorHandler>
struct ValueMaker<ErrorHandler, ConversionMode::Init>
{

};
*/
}
}
