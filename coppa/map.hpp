#pragma once
#include <mutex>

#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>

#include <type_traits>
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
template<typename Map, typename Key>
auto filter(const Map& map, Key&& addr)
{
  typename Map::base_map_type newmap;
  for(const auto& param : map)
  {
    if(boost::starts_with(param.destination, addr))
      newmap.insert(param);
  }

  return newmap;
}

template<typename Map>
class LockedParameterMap
{
    Map m_map;
    mutable std::mutex m_map_mutex;

  public:
    using base_map_type = typename Map::base_map_type;
    using value_type = typename base_map_type::value_type;
    constexpr LockedParameterMap() = default;

    operator const Map&() const
    { return m_map; }

    operator Map&()
    { return m_map; }

    auto& operator[](typename Map::size_type i) const
    { return m_map[i]; }

    auto size() const
    { return m_map.size(); }

    auto& unsafeMap()
    { return static_cast<Map&>(*this); }

    const auto& unsafeMap() const
    { return static_cast<const Map&>(*this); }

    auto mapCopy() const
    { return static_cast<const Map&>(this); }


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
    bool existing_path(Key&& address) const
    {
      std::lock_guard<std::mutex> lock(m_map_mutex);
      return m_map.existing_path(address);
    }

    template<typename Key>
    auto get(Key&& address) const
    {
      std::lock_guard<std::mutex> lock(m_map_mutex);
      return m_map.get(address);
    }

    template<typename... Args>
    void update(Args&&... args)
    {
      std::lock_guard<std::mutex> lock(m_map_mutex);
      m_map.update(std::forward<Args>(args)...);
    }

    template<typename... Args>
    void update_attributes(Args&&... args)
    {
      std::lock_guard<std::mutex> lock(m_map_mutex);
      m_map.update_attributes(std::forward<Args>(args)...);
    }

    template<typename... Args>
    void replace(Args&&... args)
    {
      std::lock_guard<std::mutex> lock(m_map_mutex);
      m_map.replace(std::forward<Args>(args)...);
    }


    template<typename Element>
    void add(Element&& e)
    {
      std::lock_guard<std::mutex> lock(m_map_mutex);
      m_map.add(e);
    }

    template<typename Key>
    void remove(Key&& k)
    {
      std::lock_guard<std::mutex> lock(m_map_mutex);
      m_map.remove(k);
    }

    template<typename Map_T>
    void merge(Map_T&& other)
    {
      std::lock_guard<std::mutex> lock(m_map_mutex);
      m_map.merge(other);
    }
};


template<typename Map>
class ConstantMap
{
  protected:
    LockedParameterMap<Map> m_map;

  public:
    bool has(const std::string& address) const
    { return m_map.has(address); }

    auto get(const std::string& address) const
    { return m_map.get(address); }

    // TODO cbegin / cend

    LockedParameterMap<Map>& safeMap()
    { return m_map; }
    const LockedParameterMap<Map>& safeMap() const
    { return m_map; }

    template<typename Map_T>
    void replace(Map_T&& map)
    { m_map = std::move(map); }
};

// Sets a value on a remote device via a protocol like OSC.
// TODO the local map shouldn't be constant ?
// Or maybe we should have the choice between "fully mirrored" where
// we only get changes via callbacks of the server
// and a "local", modifiable mirror.
template<typename Map, typename DataProtocolSender>
class SettableMap : public ConstantMap<Map>
{
  private:
    DataProtocolSender m_sender;

  public:
    void connect(const std::string& uri, int port)
    { m_sender = DataProtocolSender{uri, port}; }

    template<typename... Args>
    void set(const std::string& address, Args&&... args)
    {
      auto param = ConstantMap<Map>::get(address);
      if(param.accessmode == Access::Mode::Set
      || param.accessmode == Access::Mode::Both)
      {
        m_sender.send(address, std::forward<Args>(args)...);
      }
    }
};
}
