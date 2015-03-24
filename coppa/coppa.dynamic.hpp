#pragma once
#define coppa_dynamic
#include <coppa/coppa.hpp>
namespace coppa
{

// Static knowledge about our classes
template<typename... Args>
class StaticParameter : public AttributeAggregate<Args...>
{
  public:
    // Attributes
    const std::array<const char*, sizeof...(Args)> m_attributes{Args::name...};

    template<typename Attribute> static constexpr bool has() noexcept
    { return std::is_base_of<Attribute, StaticParameter>::value; }

    // No need to ask before in this case
    template<typename Attribute> Attribute& get() noexcept
    { return static_cast<Attribute&>(*this); }

    template<typename Attribute> const Attribute& get() const noexcept
    { return static_cast<const Attribute&>(*this); }
};

// It is possible to introduce dynamic dispatch :
class ParameterBase
{
    public:
        // Query attributes
        virtual std::vector<std::string> attributes() const noexcept = 0;
        virtual bool hasAttribute(const std::string& attr) const noexcept = 0;

        template<typename Attribute>
        Attribute* get_dyn()
        { return dynamic_cast<Attribute*>(this); }
};

template<typename... Args>
class ParameterAdapter : public ParameterBase, public StaticParameter<Args...>
{
    public:
        ParameterAdapter() = default;
        virtual std::vector<std::string> attributes() const noexcept override
        {
            using namespace std;
            vector<string> attr;
            copy(begin(StaticParameter<Args...>::m_attributes),
                 end(StaticParameter<Args...>::m_attributes),
                 back_inserter(attr));
            return attr;
        }

        virtual bool hasAttribute(const std::string& attr) const noexcept override
        {
            return find(begin(StaticParameter<Args...>::m_attributes),
                        end(StaticParameter<Args...>::m_attributes), attr)
                    != end(StaticParameter<Args...>::m_attributes);
        }
};

}
