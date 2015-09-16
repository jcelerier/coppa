#pragma once
#include <string>
#include <unordered_set>
// TODO namespace
template<typename QueryServer>
class RemoteClient
{
    typename QueryServer::connection_handler m_handler;
    std::unordered_set<std::string> m_listened;

  public:
    RemoteClient(
        typename QueryServer::connection_handler handler):
      m_handler{handler} { }

    operator typename QueryServer::connection_handler() const
    { return m_handler; }

    // For performance's sake, it would be better
    // to revert this and have a table of client id's associated to each listened parameters.
    void addListenedPath(const std::string& path)
    { m_listened.insert(path); }
    void removeListenedPath(const std::string& path)
    { m_listened.erase(path); }

    const auto& listenedPaths() const noexcept
    { return m_listened; }

    bool operator==(
        const typename QueryServer::connection_handler& h) const
    { return !m_handler.expired() && m_handler.lock() == h.lock(); }
};
