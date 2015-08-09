#pragma once
#include <string>
#include <vector>
#include <map>
#include <boost/algorithm/string.hpp>
#include <coppa/exceptions/BadRequest.hpp>

namespace coppa
{
namespace oscquery
{
   /**
   * @brief The QueryParser class
   *
   * Parse OSCQUery queries.
   * The queries are similar to the GET part of an http request.
   * i.e. /a/b?value, etc...
   */
  class QueryParser
  {
    public:
      template<typename Mapper>
      static std::string parse(const std::string& request, Mapper&& mapper)
      {
        using namespace boost;
        using namespace std;

        // Split on "?"
        vector<string> uri_tokens;
        split(uri_tokens, request, is_any_of("?"));

        // Parse the methods
        map<string, string> arguments_map;
        if(uri_tokens.size() > 1)
        {
          if(uri_tokens.size() > 2)
          {
            throw BadRequestException{"Too much '?'"};
          }

          // Then, split the &-separated arguments
          vector<string> argument_tokens;
          split(argument_tokens, uri_tokens.at(1), is_any_of("&"));

          // Finally, split these arguments at '=' and put them in a map
          for(const auto& arg : argument_tokens)
          {
            vector<string> map_tokens;
            split(map_tokens, arg, is_any_of("="));

            switch(map_tokens.size())
            {
              case 1: // &value
                arguments_map.insert({map_tokens.front(), {}});
                break;
              case 2: // &listen=true
                arguments_map.insert({map_tokens.front(), map_tokens.back()});
                break;
              default:
                throw BadRequestException{"Too much '='"};
                break;
            }
          }
        }

        return mapper(uri_tokens.at(0), arguments_map);
      }

  };
}
}
