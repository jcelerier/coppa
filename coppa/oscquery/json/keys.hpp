#pragma once
#include <coppa/oscquery/parameter.hpp>
namespace coppa
{
namespace oscquery
{
namespace JSON
{
namespace key
{
// Attributes
constexpr const char* osc_port() { return "osc_port"; }
constexpr const char* full_path() { return "full_path"; }

constexpr const char* type() { return "type"; }
constexpr const char* contents() { return "contents"; }

template<typename T>
constexpr const char* attribute();

template<> constexpr const char* attribute<Values>() { return "value"; }
template<> constexpr const char* attribute<Ranges>() { return "range"; }
template<> constexpr const char* attribute<ClipModes>() { return "clipmode"; }
template<> constexpr const char* attribute<Access>() { return "access"; }
template<> constexpr const char* attribute<Description>() { return "description"; }
template<> constexpr const char* attribute<Tags>() { return "tags"; }

template<typename T>
constexpr const char* attribute(const T&) { return attribute<T>(); }

// Commands
constexpr const char* path_added() { return "path_added"; }
constexpr const char* path_removed() { return "path_removed"; }
constexpr const char* path_changed() { return "path_changed"; }
constexpr const char* attributes_changed() { return "attributes_changed"; }

constexpr const char* paths_added() { return "paths_added"; }
constexpr const char* paths_removed() { return "paths_removed"; }
constexpr const char* paths_changed() { return "paths_changed"; }
constexpr const char* attributes_changed_array() { return "attributes_changed_array"; }
}
}
}
}
