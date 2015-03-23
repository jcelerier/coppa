#pragma once
#include <string>
#include <vector>
#include <utility>
#include <eggs/variant.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#define coppa_name(theName) static constexpr const char * name{ #theName };
#define coppa_parameter(theValue) \
    const auto& get() const { return theValue; } \
    template<typename T> void set(T&& t) { theValue = t; }


namespace coppa
{
    // Definition of standard attributes
    template<typename ValueType>
    struct SimpleValue
    {
            coppa_name(SimpleValue)
            ValueType value;
    };

    using Tag = std::string;
    struct Tags
    {
            coppa_name(Tags)
            std::vector<Tag> tags;
    };

    struct Alias
    {
            coppa_name(Alias)
            std::string alias;
    };

    struct Description
    {
            coppa_name(Description)
            std::string description;
    };
    enum class AccessMode { None = 0, Get = 1, Set = 2, Both = 3 };
    enum class ClipMode { None, Low, High, Both };

    class ParameterBase
    {
        public:
            // Query attributes
            virtual std::vector<std::string> attributes() const noexcept = 0;
            virtual bool hasAttribute(const std::string& attr) const noexcept = 0;

            template<typename Attribute>
            Attribute* get_dyn()
            { return dynamic_cast<Attribute*>(this); }

            // Destination
            std::string destination;
            const std::string& getDestination() const
            { return destination; }

            // Access mode
            AccessMode accessMode{AccessMode::None};
    };

    template<typename... Args>
    class Attributes : public Args...
    {
        public:
            std::array<const char*, sizeof...(Args)> m_attributes{Args::name...};

            template<typename Attribute> static constexpr bool has() noexcept
            { return std::is_base_of<Attribute, Attributes>::value; }

            // No need to ask before in this case
            template<typename Attribute> Attribute& get() noexcept
            { return static_cast<Attribute&>(*this); }
    };

    template<typename... Args>
    class ParameterAdapter : public ParameterBase, public Attributes<Args...>
    {
        public:
            ParameterAdapter() = default;
            virtual std::vector<std::string> attributes() const noexcept override
            {
                using namespace std;
                vector<string> attr;
                copy(begin(Attributes<Args...>::m_attributes),
                     end(Attributes<Args...>::m_attributes),
                     back_inserter(attr));
                return attr;
            }

            virtual bool hasAttribute(const std::string& attr) const noexcept override
            {
                return find(begin(Attributes<Args...>::m_attributes),
                            end(Attributes<Args...>::m_attributes), attr)
                        != end(Attributes<Args...>::m_attributes);
            }
    };

    using Generic = const char*;
    using Variant = eggs::variant<int, float, bool, std::string, Generic>;

    namespace bmi = boost::multi_index;
    template<typename ParameterType>
    using ParameterMapType = bmi::multi_index_container<
        ParameterType,
        bmi::indexed_by<
            bmi::hashed_unique<
                bmi::const_mem_fun<
                    ParameterBase,
                    std::string const&,
                    &ParameterBase::getDestination>>>>;

    // Get all the parameters whose address begins with addr
    template<typename MapType>
    auto filter(const MapType& map, std::string addr)
    {
        using namespace std;
        MapType newmap;
        for(const auto& param : map)
        {
            if(boost::starts_with(param.destination, addr))
                newmap.insert(param);
        }

        return newmap;
    }

    // TODO rebase with new root algorithm

}
