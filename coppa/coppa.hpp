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

namespace coppa
{
template<typename ValueType>
struct SimpleValue
{
    coppa_name(SimpleValue);
    ValueType value;
};

using Tag = std::string;
struct Tags
{
    coppa_name(Tags);
    std::vector<Tag> tags;
};

struct Alias
{
    coppa_name(Alias);
    std::string alias;
};

struct Description
{
    coppa_name(Description);
    std::string description;
};

struct RepetitionFilter
{
    coppa_name(RepetitionFilter);
    bool repetitionFilter;
};


template<typename ValueType>
struct Interval
{
    enum class Type { OpenOpen, OpenClosed, ClosedOpen, ClosedClosed } type;
    std::pair<ValueType, ValueType> range;
};

template<typename ValueType> using Enum = std::vector<ValueType>;
enum class BoundingMode { Free, Clip, Wrap, Fold };

template<typename ValueType>
using DomainType = std::vector<eggs::variant<Interval<ValueType>, Enum<ValueType>>>;

template<typename DomainValueType,
         typename InfComparator,
         typename EqComparator>
class Bounds
{
    public:
        coppa_name(Bounds);

        DomainType<DomainValueType> domain;
        BoundingMode lower_bound = BoundingMode::Free;
        BoundingMode upper_bound = BoundingMode::Free;

        template<typename ValueType>
        bool valueIsInBounds(ValueType val)
        {
            using namespace eggs::variants;
            for(auto&& subdomain : domain)
            {
                if(const auto& interval = get<Interval<DomainValueType>>(subdomain))
                {
                    if(InfComparator::operatorInf(interval.range.first, val))
                    {
                    }
                }
                else
                {
                    const auto& enumeration = get<Enum<DomainValueType>>(subdomain);
                    for(auto enum_val : enumeration)
                    {
                        if(EqComparator::operatorEq(val, enum_val))
                        {
                        }
                    }
                }
            }
        }
};

template<class ValueType>
class StandardComparator
{
    static bool operatorInf(const ValueType& lhs, const ValueType& rhs)
    { return lhs < rhs; }
    static bool operatorEq(const ValueType& lhs, const ValueType& rhs)
    { return lhs == rhs; }
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


using MinuitParameter = ParameterAdapter<SimpleValue<Variant>,
                                         Tags,
                                         Alias,
                                         RepetitionFilter,
                                         Bounds<Variant,
                                            StandardComparator<Variant>,
                                            StandardComparator<Variant>>>;

namespace bmi = boost::multi_index;
template<typename ParameterType>
using ParameterMapType = bmi::multi_index_container<
    ParameterType,
    bmi::indexed_by<
        bmi::hashed_unique<
            bmi::const_mem_fun<ParameterBase,
                               std::string const&,
                               &ParameterBase::getDestination>
        >
    >
>;

// Get all the parameters whose addres begins with addr
template<typename ParameterMapType_T>
auto filter(const ParameterMapType_T& map, std::string addr)
{
    using namespace std;
    ParameterMapType_T newmap;
    for(const auto& param : map)
    {
        if(boost::starts_with(param.destination, addr))
            newmap.insert(param);
    }

    return newmap;
}

}
