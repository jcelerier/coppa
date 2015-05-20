#pragma once
#include <coppa/coppa.hpp>
#include <oscpack/osc/OscOutboundPacketStream.h>
#include <cstring>
namespace osc
{
    class MessageGenerator
    {
        public:
            MessageGenerator() = default;
            MessageGenerator(unsigned int c):
                buffer(c),
                p(buffer.data(), c)
            {
            }

            template <typename... T>
            MessageGenerator(const std::string& name, const T&... args)
            {
                operator()(name, args...);
            }

            template <typename... T>
            MessageGenerator(unsigned int c, const std::string& name, const T&... args):
                buffer(c),
                p(buffer.data(), c)
            {
                operator()(name, args...);
            }


            template <typename... T>
            const osc::OutboundPacketStream&  operator()(const std::string& name, const T&... args)
            {
                p.Clear();
                p << osc::BeginBundleImmediate << osc::BeginMessage( name.c_str() );
                subfunc(args...);

                return p;
            }

            const osc::OutboundPacketStream&  operator()(const std::string& name,
                                                         const std::vector<coppa::Variant>& values)
            {
                using namespace eggs::variants;

                p.Clear();
                p << osc::BeginBundleImmediate << osc::BeginMessage( name.c_str() );
                for(const auto& val : values)
                {
                    switch(val.which())
                    {
                        case 0:  p << get<int>(val); break;
                        case 1:  p << get<float>(val); break;
                            //case 2: array.add(get<bool>(val)); break;
                        case 3:  p << osc::Symbol(get<std::string>(val).c_str()); break;
                        case 4:
                        {
                            const char* str = get<const char*>(val);
                            osc::Blob b(str, std::strlen(str)); // todo : use Generic instead and convert to hex / base64
                            p << b;
                            break;
                        }
                    }
                }
                p << osc::EndMessage << osc::EndBundle;

                return p;
            }

            const osc::OutboundPacketStream& stream() const
            {
                return p;
            }

        private:
            void subfunc()
            {
                p << osc::EndMessage << osc::EndBundle;
            }

            template <typename Arg1, typename... Args>
            void subfunc(Arg1&& arg1, Args&&... args)
            {
                p << arg1;
                subfunc(args...);
            }

            std::vector<char> buffer{std::vector<char>(1024)};
            osc::OutboundPacketStream p{osc::OutboundPacketStream(buffer.data(), buffer.size())};
    };
}
