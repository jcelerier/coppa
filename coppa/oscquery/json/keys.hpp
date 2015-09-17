#pragma once
#include <coppa/oscquery/parameter.hpp>
namespace coppa
{
namespace oscquery
{
namespace json
{
namespace key
{
// Attributes
constexpr const char* osc_port() { return "OSC_PORT"; }
constexpr const char* full_path() { return "FULL_PATH"; }

constexpr const char* type() { return "TYPE"; }
constexpr const char* contents() { return "CONTENTS"; }

template<typename T>
constexpr const char* attribute();

template<> constexpr const char* attribute<Values>() { return "VALUE"; }
template<> constexpr const char* attribute<Ranges>() { return "RANGE"; }
template<> constexpr const char* attribute<ClipModes>() { return "CLIPMODE"; }
template<> constexpr const char* attribute<Access>() { return "ACCESS"; }
template<> constexpr const char* attribute<Description>() { return "DESCRIPTION"; }
template<> constexpr const char* attribute<Tags>() { return "TAGS"; }

template<typename T>
constexpr const char* attribute(const T&) { return attribute<T>(); }

// Commands
constexpr const char* path_added() { return "PATH_ADDED"; }
constexpr const char* path_removed() { return "PATH_REMOVED"; }
constexpr const char* path_changed() { return "PATH_CHANGED"; }
constexpr const char* attributes_changed() { return "ATTRIBUTES_CHANGED"; }

constexpr const char* paths_added() { return "PATHS_ADDED"; }
constexpr const char* paths_removed() { return "PATHS_REMOVED"; }
constexpr const char* paths_changed() { return "PATHS_CHANGED"; }
constexpr const char* attributes_changed_array() { return "ATTRIBUTES_CHANGED_ARRAY"; }
}
}
}
}
