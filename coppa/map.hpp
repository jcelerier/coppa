#pragma once
#include <mutex>

#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
namespace coppa
{
// We make maps on parameter with a destination
namespace bmi = boost::multi_index;
template<typename ParameterType>
using ParameterMapType = bmi::multi_index_container<
ParameterType,
bmi::indexed_by<
bmi::ordered_unique< // TODO compare performance with ordered_unique
bmi::member<
Destination,
std::string,
&Destination::destination>>,
bmi::random_access<>>>;

// Get all the parameters whose address begins with addr
// TODO make an algorithm to rebase a map with a new root. (the inverse of this)
template<typename MapType, typename Key>
auto filter(const MapType& map, Key&& addr)
{
  MapType newmap;
  for(const auto& param : map)
  {
    if(boost::starts_with(param.destination, addr))
      newmap.add(param);
  }

  return newmap;
}

template<typename Map>
class SimpleParameterMap
{
    Map m_map;

  public:
    using map_type = Map;
    using size_type = typename Map::size_type;

    constexpr SimpleParameterMap() = default;

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
    auto get(Key&& address) const
    {
      return *m_map.template get<0>().find(address);
    }

    operator const Map&() const
    {
      return m_map;
    }

    const auto& map_impl() const
    {
      return static_cast<const Map&>(*this);
    }

    auto& operator[](size_type i) const
    {
      return m_map.template get<1>()[i];
    }

    auto size() const
    {
      return m_map.size();
    }

    auto begin() const
    {
      return m_map.begin();
    }

    auto end() const
    {
      return m_map.end();
    }

    template<typename Key, typename Updater>
    auto update(Key&& address, Updater&& updater)
    {
      auto& param_index = m_map.template get<0>();
      decltype(auto) param = param_index.find(address);
      param_index.modify(param, updater);
    }

    template<typename Element>
    auto add(Element&& e)
    {
      m_map.insert(e);
    }

    template<typename Key>
    auto remove(Key&& k)
    {
      // Remove the path and its children
      for(auto&& elt : filter(*this, k))
      {
        m_map.template get<0>().erase(elt.destination);
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

template<typename Map>
class LockedParameterMap
{
    Map m_map;
    mutable std::mutex m_map_mutex;

  public:
    using map_type = Map;
    constexpr LockedParameterMap() = default;

    template<typename Map_T>
    LockedParameterMap& operator=(Map_T&& map)
    {
      std::lock_guard<std::mutex> lock(m_map_mutex);
      m_map = std::move(map);
      return *this;
    }

    template<typename Key>
    bool has(Key&& address) const
    {
      std::lock_guard<std::mutex> lock(m_map_mutex);
      return m_map.has(address);
    }

    template<typename Key>
    auto get(Key&& address) const
    {
      std::lock_guard<std::mutex> lock(m_map_mutex);
      return m_map.get(address);
    }

    operator const Map&() const
    {
      return m_map;
    }

    auto& operator[](typename Map::size_type i) const
    {
      return m_map[i];
    }

    auto size() const
    {
      return m_map.size();
    }

    auto begin() const
    {
      return m_map.begin();
    }

    auto end() const
    {
      return m_map.end();
    }

    const auto& unsafeMap() const
    {
      return static_cast<const Map&>(*this);
    }

    template<typename Key, typename Updater>
    auto update(Key&& address, Updater&& updater)
    {
      std::unique_lock<std::mutex> lock(m_map_mutex);
      m_map.update(address, updater);

      return lock;
    }

    template<typename Element>
    auto add(Element&& e)
    {
      std::unique_lock<std::mutex> lock(m_map_mutex);
      m_map.add(e);

      return lock;
    }

    template<typename Key>
    auto remove(Key&& k)
    {
      std::unique_lock<std::mutex> lock(m_map_mutex);
      m_map.remove(k);

      return lock;
    }

    template<typename Map_T>
    void merge(Map_T&& other)
    {
      std::unique_lock<std::mutex> lock(m_map_mutex);
      m_map.merge(other);
    }
};

}
