#pragma once
#include <coppa/oscquery/parameter.hpp>
#include <type_traits>

namespace coppa
{
namespace oscquery
{
using ParameterMap = ParameterMapType<Parameter>;

template<typename Map>
class SimpleParameterMap
{
    Map m_map;
    static auto makeRootNode()
    {
      typename Map::value_type root;
      root.description = std::string("root node");
      root.destination = std::string("/");
      root.accessmode = Access::Mode::None;
      return root;
    }

    auto make_update_fun()
    {
      return [] (auto&& ) { };
    }
    template<typename Arg>
    auto make_update_fun(Arg&& arg)
    {
      return [&] (auto&& p) {
        static_cast<Arg&&>(p) = arg;
      };
    }

    template<typename Arg, typename... Args>
    auto make_update_fun(Arg&& arg, Args&&... args)
    {
      return [&] (auto&& p) {
        make_update_fun(std::forward<Args>(args)...)(p);
        static_cast<Arg&&>(p) = arg;
      };
    }

  public:
    using base_map_type = Map;
    using size_type = typename Map::size_type;

    constexpr SimpleParameterMap()
    { add(makeRootNode()); }

    SimpleParameterMap& operator=(Map&& map)
    {
      m_map = std::move(map);
      return *this;
    }

    template<typename Key>
    bool has(Key&& address) const
    {
      decltype(auto) index = m_map.template get<0>();
      return index.find(address) != index.end();
    }


    template<typename Key>
    bool existing_path(Key&& address) const
    {
      return std::any_of(
            std::begin(m_map),
            std::end(m_map),
            [&] (const typename Map::value_type& param) { return boost::starts_with(param.destination, address); });
    }

    template<typename Key>
    auto get(Key&& address) const
    { return *m_map.template get<0>().find(address); }

    operator const Map&() const
    { return m_map; }

    const auto& map_impl() const
    { return static_cast<const Map&>(*this); }

    auto& operator[](size_type i) const
    { return m_map.template get<1>()[i]; }

    auto size() const
    { return m_map.size(); }

    auto begin() const
    { return m_map.begin(); }

    auto end() const
    { return m_map.end(); }


    template<typename Key,
             typename Updater>
    auto update(Key&& address, Updater&& updater)
    {
      auto& param_index = m_map.template get<0>();
      decltype(auto) param = param_index.find(address);
      param_index.modify(param, std::forward<Updater>(updater));
    }

    template<typename Key,
             typename... Args>
    auto update_attributes(Key&& address, Args&&... args)
    {
      update(std::forward<Key>(address),
             make_update_fun(std::forward<Args>(args)...));
    }

    template<typename Element>
    auto replace(const Element& replacement)
    {
      auto& param_index = m_map.template get<0>();
      param_index.replace(param_index.find(replacement.destination), replacement);
    }


    // TODO rename in insert + add insert_and_assign
    // See boost doc with rollback, too.
    template<typename Element>
    auto add(Element&& e)
    { m_map.insert(e); }

    template<typename Key>
    auto remove(Key&& k)
    {
      // Remove the path and its children
      for(auto&& elt : filter(*this, k))
      {
        m_map.template get<0>().erase(elt.destination);
      }

      // If the root node was removed we reinstate it
      if(size() == 0)
      {
        add(makeRootNode());
      }
    }

    template<typename Map_T>
    void merge(Map_T&& other)
    {
      for(auto&& elt : other)
      {
        m_map.insert(elt);
      }
    }
};
}
}
